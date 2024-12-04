/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

namespace
{
    const size_t baseFontSize = 96;

    const uint8_t buttonGoodReleasedColor = 56;
    const uint8_t buttonGoodPressedColor = 62;
    const uint8_t buttonEvilReleasedColor = 30;
    const uint8_t buttonEvilPressedColor = 36;
    const uint8_t buttonContourColor = 10;
    const fheroes2::Point buttonFontOffset{ -1, 0 };

    void updateNormalFontLetterShadow( fheroes2::Image & letter )
    {
        fheroes2::updateShadow( letter, { -1, 2 }, 2, false );
    }

    void updateSmallFontLetterShadow( fheroes2::Image & letter )
    {
        fheroes2::updateShadow( letter, { -1, 1 }, 2, true );
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

    void generateCP1250Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );

            const fheroes2::Sprite firstSprite{ icnVsSprite[icnId][0] };
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, firstSprite );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // Uppercase S with caron. Generate accent for further use.
            font[138 - 32].resize( font[83 - 32].width(), font[83 - 32].height() + 3 );
            font[138 - 32].reset();
            fheroes2::Copy( font[83 - 32], 0, 0, font[138 - 32], 0, 3, font[83 - 32].width(), font[83 - 32].height() );
            fheroes2::Copy( font[65], 1, 1, font[138 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[138 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[138 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[65], 7, 1, font[138 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[138 - 32], 6, 1, 1, 1 );
            font[138 - 32].setPosition( font[83 - 32].x(), font[83 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[138 - 32] );

            // Uppercase S with acute. Generate accent for further use.
            font[140 - 32].resize( font[83 - 32].width(), font[83 - 32].height() + 3 );
            font[140 - 32].reset();
            fheroes2::Copy( font[83 - 32], 0, 0, font[140 - 32], 0, 3, font[83 - 32].width(), font[83 - 32].height() );
            fheroes2::Copy( font[111 - 32], 2, 0, font[140 - 32], 4, 0, 3, 2 );
            fheroes2::FillTransform( font[140 - 32], 4, 0, 1, 1, 1 );
            fheroes2::FillTransform( font[140 - 32], 6, 1, 1, 1, 1 );
            font[140 - 32].setPosition( font[83 - 32].x(), font[83 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[140 - 32] );

            // Uppercase T with caron
            font[141 - 32].resize( font[84 - 32].width(), font[84 - 32].height() + 3 );
            font[141 - 32].reset();
            fheroes2::Copy( font[84 - 32], 0, 0, font[141 - 32], 0, 3, font[84 - 32].width(), font[84 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[141 - 32], 5, 0, 3, 2 );
            font[141 - 32].setPosition( font[84 - 32].x(), font[84 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[141 - 32] );

            // Uppercase Z with caron
            font[142 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[142 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[142 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[142 - 32], 5, 0, 3, 2 );
            font[142 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[142 - 32] );

            // Uppercase Z with acute
            font[143 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[143 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[143 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[143 - 32], 4, 0, 3, 2 );
            font[143 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[143 - 32] );

            // Lowercase s with caron
            font[154 - 32].resize( font[115 - 32].width(), font[115 - 32].height() + 3 );
            font[154 - 32].reset();
            fheroes2::Copy( font[115 - 32], 0, 0, font[154 - 32], 0, 3, font[115 - 32].width(), font[115 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[154 - 32], 3, 0, 3, 2 );
            font[154 - 32].setPosition( font[115 - 32].x(), font[115 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[154 - 32] );

            // Lowercase s with acute
            font[156 - 32].resize( font[115 - 32].width(), font[115 - 32].height() + 3 );
            font[156 - 32].reset();
            fheroes2::Copy( font[115 - 32], 0, 0, font[156 - 32], 0, 3, font[115 - 32].width(), font[115 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[156 - 32], 4, 0, 3, 2 );
            font[156 - 32].setPosition( font[115 - 32].x(), font[115 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[156 - 32] );

            // Lowercase t with caron
            font[157 - 32].resize( font[116 - 32].width(), font[116 - 32].height() + 1 );
            font[157 - 32].reset();
            fheroes2::Copy( font[116 - 32], 0, 0, font[157 - 32], 0, 1, font[116 - 32].width(), font[116 - 32].height() );
            fheroes2::Copy( font[65], 1, 1, font[157 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[157 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[157 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[157 - 32], 5, 0, 1, 1 );
            font[157 - 32].setPosition( font[116 - 32].x(), font[116 - 32].y() - 1 );
            updateNormalFontLetterShadow( font[157 - 32] );

            // Lowercase z with caron
            font[158 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 3 );
            font[158 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[158 - 32], 0, 3, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[158 - 32], 4, 0, 3, 2 );
            font[158 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[158 - 32] );

            // Lowercase z with acute
            font[159 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 3 );
            font[159 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[159 - 32], 0, 3, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[159 - 32], 4, 0, 3, 2 );
            font[159 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[159 - 32] );

            // Uppercase L with stroke
            font[163 - 32].resize( font[76 - 32].width(), font[76 - 32].height() + 3 );
            font[163 - 32].reset();
            fheroes2::Copy( font[76 - 32], 0, 0, font[163 - 32], 0, 0, font[76 - 32].width(), font[76 - 32].height() );
            // Stroke diacritic.
            fheroes2::Copy( font[76 - 32], 1, 1, font[163 - 32], 6, 5, 1, 1 );
            fheroes2::Copy( font[76 - 32], 1, 1, font[163 - 32], 7, 4, 1, 1 );
            fheroes2::Copy( font[76 - 32], 1, 1, font[163 - 32], 8, 3, 1, 1 );
            fheroes2::Copy( font[76 - 32], 2, 1, font[163 - 32], 6, 6, 1, 1 );
            fheroes2::Copy( font[76 - 32], 2, 1, font[163 - 32], 7, 5, 1, 1 );
            fheroes2::Copy( font[76 - 32], 2, 1, font[163 - 32], 8, 4, 1, 1 );
            font[163 - 32].setPosition( font[76 - 32].x(), font[76 - 32].y() );
            updateNormalFontLetterShadow( font[163 - 32] );

            // Uppercase A with ogonek. Generate ogonek for further use.
            font[165 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[165 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[165 - 32], 0, 0, font[65 - 32].width(), font[65 - 32].height() );
            // Ogonek diacritic.
            fheroes2::Copy( font[97 - 32], 6, 5, font[165 - 32], 12, 11, 1, 1 );
            fheroes2::Copy( font[97 - 32], 6, 5, font[165 - 32], 11, 12, 1, 1 );
            fheroes2::Copy( font[97 - 32], 6, 5, font[165 - 32], 12, 13, 2, 1 );
            fheroes2::Copy( font[97 - 32], 6, 5, font[165 - 32], 14, 12, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 3, font[165 - 32], 11, 11, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 3, font[165 - 32], 12, 12, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 3, font[165 - 32], 11, 13, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 3, font[165 - 32], 14, 13, 1, 1 );
            font[165 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() );
            updateNormalFontLetterShadow( font[165 - 32] );
            // Remove unnecessary shadows
            fheroes2::FillTransform( font[165 - 32], 10, 13, 1, 1, 1 );
            fheroes2::FillTransform( font[165 - 32], 11, 14, 1, 1, 1 );
            fheroes2::FillTransform( font[165 - 32], 10, 15, 1, 1, 1 );
            fheroes2::FillTransform( font[165 - 32], 13, 15, 1, 1, 1 );

            // Uppercase Z with dot above. Generate dot for further use.
            font[175 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[175 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[175 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[122 - 32], 7, 5, font[175 - 32], 5, 0, 2, 1 );
            fheroes2::Copy( font[122 - 32], 6, 1, font[175 - 32], 5, 1, 2, 1 );
            font[175 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[175 - 32] );

            // Lowercase l with stroke
            font[179 - 32].resize( font[108 - 32].width(), font[108 - 32].height() + 3 );
            font[179 - 32].reset();
            fheroes2::Copy( font[108 - 32], 0, 0, font[179 - 32], 0, 0, font[108 - 32].width(), font[108 - 32].height() );
            // Stroke diacritic.
            fheroes2::Copy( font[108 - 32], 3, 1, font[179 - 32], 4, 3, 1, 1 );
            fheroes2::Copy( font[108 - 32], 2, 1, font[179 - 32], 4, 4, 1, 1 );
            fheroes2::Copy( font[108 - 32], 3, 1, font[179 - 32], 2, 6, 1, 1 );
            fheroes2::Copy( font[108 - 32], 2, 1, font[179 - 32], 1, 6, 1, 1 );
            fheroes2::Copy( font[108 - 32], 3, 0, font[179 - 32], 2, 7, 1, 1 );
            fheroes2::Copy( font[108 - 32], 1, 1, font[179 - 32], 1, 7, 1, 1 );
            font[179 - 32].setPosition( font[108 - 32].x(), font[108 - 32].y() );
            updateNormalFontLetterShadow( font[179 - 32] );

            // Lowercase a with ogonek
            font[185 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[185 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[185 - 32], 0, 0, font[97 - 32].width(), font[97 - 32].height() );
            font[185 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() );
            updateNormalFontLetterShadow( font[185 - 32] );
            // Shadows are already made for the ogonek.
            fheroes2::Copy( font[165 - 32], 10, 11, font[185 - 32], 5, 7, 5, 5 );

            // Uppercase L with caron (NOT an uppercase Y with diaeresis)
            font[188 - 32].resize( font[76 - 32].width(), font[76 - 32].height() );
            font[188 - 32].reset();
            fheroes2::Copy( font[76 - 32], 0, 0, font[188 - 32], 0, 0, font[76 - 32].width(), font[76 - 32].height() );
            fheroes2::Copy( font[65], 1, 1, font[188 - 32], 9, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[188 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[188 - 32], 10, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[188 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[188 - 32], 8, 1, 1, 1 );
            font[188 - 32].setPosition( font[76 - 32].x(), font[76 - 32].y() );
            updateNormalFontLetterShadow( font[188 - 32] );

            // Lowercase l with caron (NOT an uppercase L with caron)
            font[190 - 32].resize( font[108 - 32].width() + 2, font[108 - 32].height() );
            font[190 - 32].reset();
            fheroes2::Copy( font[108 - 32], 0, 0, font[190 - 32], 0, 0, font[108 - 32].width(), font[108 - 32].height() );
            fheroes2::Copy( font[188 - 32], 8, 0, font[190 - 32], 4, 0, 3, 2 );
            font[190 - 32].setPosition( font[108 - 32].x(), font[108 - 32].y() );
            updateNormalFontLetterShadow( font[190 - 32] );

            // Lowercase z with dot above
            font[191 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 3 );
            font[191 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[191 - 32], 0, 3, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[191 - 32], 4, 0, 2, 2 );
            font[191 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[191 - 32] );

            // Uppercase R with acute
            font[192 - 32].resize( font[82 - 32].width(), font[82 - 32].height() + 3 );
            font[192 - 32].reset();
            fheroes2::Copy( font[82 - 32], 0, 0, font[192 - 32], 0, 3, font[82 - 32].width(), font[82 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[192 - 32], 7, 0, 3, 2 );
            font[192 - 32].setPosition( font[82 - 32].x(), font[82 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[192 - 32] );

            // Uppercase A with acute
            font[193 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[193 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[193 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[193 - 32], 7, 0, 3, 2 );
            font[193 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[193 - 32] );

            // Uppercase A with circumflex
            font[194 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[194 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 7, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 0, font[194 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 0, font[194 - 32], 9, 0, 1, 1 );
            font[194 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[194 - 32] );

            // Uppercase A with diaeresis
            font[196 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[196 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[196 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[196 - 32], 5, 0, 2, 2 );
            fheroes2::Copy( font[175 - 32], 5, 0, font[196 - 32], 10, 0, 2, 2 );
            font[196 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[196 - 32] );

            // Uppercase L with acute
            font[197 - 32].resize( font[76 - 32].width(), font[76 - 32].height() + 3 );
            font[197 - 32].reset();
            fheroes2::Copy( font[76 - 32], 0, 0, font[197 - 32], 0, 3, font[76 - 32].width(), font[76 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[197 - 32], 8, 0, 3, 2 );
            font[197 - 32].setPosition( font[76 - 32].x(), font[76 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[197 - 32] );

            // Uppercase C with acute
            font[198 - 32].resize( font[67 - 32].width(), font[67 - 32].height() + 3 );
            font[198 - 32].reset();
            fheroes2::Copy( font[67 - 32], 0, 0, font[198 - 32], 0, 3, font[67 - 32].width(), font[67 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[198 - 32], 8, 0, 3, 2 );
            font[198 - 32].setPosition( font[67 - 32].x(), font[67 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[198 - 32] );

            // Uppercase C with caron
            font[200 - 32].resize( font[67 - 32].width(), font[67 - 32].height() + 3 );
            font[200 - 32].reset();
            fheroes2::Copy( font[67 - 32], 0, 0, font[200 - 32], 0, 3, font[67 - 32].width(), font[67 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[200 - 32], 7, 0, 3, 2 );
            font[200 - 32].setPosition( font[67 - 32].x(), font[67 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[200 - 32] );

            // Uppercase E with acute
            font[201 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[201 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[201 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[201 - 32], 6, 0, 3, 2 );
            font[201 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[201 - 32] );

            // Uppercase E with ogonek
            font[202 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[202 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[202 - 32], 0, 0, font[69 - 32].width(), font[69 - 32].height() );
            font[202 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() );
            updateNormalFontLetterShadow( font[202 - 32] );
            // Shadows are already made for the ogonek.
            fheroes2::Copy( font[165 - 32], 10, 11, font[202 - 32], 5, 11, 5, 5 );

            // Uppercase E with caron
            font[204 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[204 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[204 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[204 - 32], 5, 0, 3, 2 );
            font[204 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[204 - 32] );

            // Uppercase I with acute
            font[205 - 32].resize( font[73 - 32].width(), font[73 - 32].height() + 3 );
            font[205 - 32].reset();
            fheroes2::Copy( font[73 - 32], 0, 0, font[205 - 32], 0, 3, font[73 - 32].width(), font[73 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[205 - 32], 4, 0, 3, 2 );
            font[205 - 32].setPosition( font[73 - 32].x(), font[73 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[205 - 32] );

            // Uppercase D with caron
            font[207 - 32].resize( font[68 - 32].width(), font[68 - 32].height() + 3 );
            font[207 - 32].reset();
            fheroes2::Copy( font[68 - 32], 0, 0, font[207 - 32], 0, 3, font[68 - 32].width(), font[68 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[207 - 32], 5, 0, 3, 2 );
            font[207 - 32].setPosition( font[68 - 32].x(), font[68 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[207 - 32] );

            // Uppercase N with acute
            font[209 - 32].resize( font[78 - 32].width(), font[78 - 32].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[78 - 32], 0, 0, font[209 - 32], 0, 3, font[78 - 32].width(), font[78 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[209 - 32], 8, 0, 3, 2 );
            font[209 - 32].setPosition( font[78 - 32].x(), font[78 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[209 - 32] );

            // Uppercase N with caron
            font[210 - 32].resize( font[78 - 32].width(), font[78 - 32].height() + 3 );
            font[210 - 32].reset();
            fheroes2::Copy( font[78 - 32], 0, 0, font[210 - 32], 0, 3, font[78 - 32].width(), font[78 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[210 - 32], 7, 0, 3, 2 );
            font[210 - 32].setPosition( font[78 - 32].x(), font[78 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[210 - 32] );

            // Uppercase O with acute
            font[211 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[211 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[211 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[211 - 32], 8, 0, 3, 2 );
            font[211 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[211 - 32] );

            // Uppercase O with circumflex
            font[212 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[212 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[212 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[65], 1, 1, font[212 - 32], 6, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[212 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[212 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[212 - 32], 9, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[212 - 32], 10, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[212 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[212 - 32], 10, 0, 1, 1 );
            font[212 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[212 - 32] );

            // Uppercase O with double acute
            font[213 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[213 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[213 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[213 - 32], 5, 0, 3, 2 );
            fheroes2::Copy( font[140 - 32], 4, 0, font[213 - 32], 9, 0, 3, 2 );
            font[213 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[213 - 32] );

            // Uppercase O with diaeresis
            font[214 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[214 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[214 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[214 - 32], 5, 0, 2, 2 );
            fheroes2::Copy( font[175 - 32], 5, 0, font[214 - 32], 10, 0, 2, 2 );
            font[214 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[214 - 32] );

            // Uppercase R with caron
            font[216 - 32].resize( font[82 - 32].width(), font[82 - 32].height() + 3 );
            font[216 - 32].reset();
            fheroes2::Copy( font[82 - 32], 0, 0, font[216 - 32], 0, 3, font[82 - 32].width(), font[82 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[216 - 32], 5, 0, 3, 2 );
            font[216 - 32].setPosition( font[82 - 32].x(), font[82 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[216 - 32] );

            // Uppercase U with ring above
            font[217 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 4 );
            font[217 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[217 - 32], 0, 4, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[80], 5, 6, font[217 - 32], 5, 0, 4, 1 );
            fheroes2::Copy( font[80], 5, 6, font[217 - 32], 5, 2, 4, 1 );
            fheroes2::Copy( font[84], 1, 0, font[217 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[84], 1, 0, font[217 - 32], 8, 1, 1, 1 );
            font[217 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[217 - 32] );

            // Uppercase U with acute
            font[218 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[218 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[218 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[218 - 32], 6, 0, 3, 2 );
            font[218 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[218 - 32] );

            // Uppercase U with double acute
            font[219 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[219 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[219 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[219 - 32], 4, 0, 3, 2 );
            fheroes2::Copy( font[140 - 32], 4, 0, font[219 - 32], 8, 0, 3, 2 );
            font[219 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[219 - 32] );

            // Uppercase U with diaeresis
            font[220 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[220 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[220 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[220 - 32], 4, 0, 2, 2 );
            fheroes2::Copy( font[175 - 32], 5, 0, font[220 - 32], 9, 0, 2, 2 );
            font[220 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[220 - 32] );

            // Uppercase Y with acute
            font[221 - 32].resize( font[89 - 32].width(), font[89 - 32].height() + 3 );
            font[221 - 32].reset();
            fheroes2::Copy( font[89 - 32], 0, 0, font[221 - 32], 0, 3, font[89 - 32].width(), font[89 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[221 - 32], 7, 0, 3, 2 );
            font[221 - 32].setPosition( font[89 - 32].x(), font[89 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[221 - 32] );

            // Lowercase r with acute
            font[224 - 32].resize( font[114 - 32].width(), font[114 - 32].height() + 3 );
            font[224 - 32].reset();
            fheroes2::Copy( font[114 - 32], 0, 0, font[224 - 32], 0, 3, font[114 - 32].width(), font[114 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[224 - 32], 4, 0, 3, 2 );
            font[224 - 32].setPosition( font[114 - 32].x(), font[114 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[224 - 32] );

            // Lowercase a with acute
            font[225 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[225 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[225 - 32], 3, 0, 3, 2 );
            font[225 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[225 - 32] );

            // Lowercase a with circumflex
            font[226 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[226 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[226 - 32], 3, 0, 3, 2 );
            font[226 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[226 - 32] );

            // Lowercase a with diaeresis
            font[228 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[228 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[228 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[228 - 32], 2, 0, 2, 2 );
            fheroes2::Copy( font[175 - 32], 5, 0, font[228 - 32], 6, 0, 2, 2 );
            font[228 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[228 - 32] );

            // Lowercase l with acute
            font[229 - 32].resize( font[108 - 32].width(), font[108 - 32].height() + 3 );
            font[229 - 32].reset();
            fheroes2::Copy( font[108 - 32], 0, 0, font[229 - 32], 0, 3, font[108 - 32].width(), font[108 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[229 - 32], 2, 0, 3, 2 );
            font[229 - 32].setPosition( font[108 - 32].x(), font[108 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[229 - 32] );

            // Lowercase c with acute
            font[230 - 32].resize( font[99 - 32].width(), font[99 - 32].height() + 3 );
            font[230 - 32].reset();
            fheroes2::Copy( font[99 - 32], 0, 0, font[230 - 32], 0, 3, font[99 - 32].width(), font[99 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[230 - 32], 4, 0, 3, 2 );
            font[230 - 32].setPosition( font[99 - 32].x(), font[99 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[230 - 32] );

            // Lowercase c with caron
            font[232 - 32].resize( font[99 - 32].width(), font[99 - 32].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[99 - 32], 0, 0, font[232 - 32], 0, 3, font[99 - 32].width(), font[99 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[232 - 32], 4, 0, 3, 2 );
            font[232 - 32].setPosition( font[99 - 32].x(), font[99 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[232 - 32] );

            // Lowercase e with acute
            font[233 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[233 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[233 - 32], 4, 0, 3, 2 );
            font[233 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[233 - 32] );

            // Lowercase e with ogonek
            font[234 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[234 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[234 - 32], 0, 0, font[101 - 32].width(), font[101 - 32].height() );
            font[234 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() );
            updateNormalFontLetterShadow( font[234 - 32] );
            // Shadows are already made for the ogonek.
            fheroes2::Copy( font[165 - 32], 10, 11, font[234 - 32], 3, 7, 5, 5 );

            // Lowercase e with caron
            font[236 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[236 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[236 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[236 - 32], 4, 0, 3, 2 );
            font[236 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[236 - 32] );

            // Lowercase i with acute
            font[237 - 32].resize( font[105 - 32].width(), font[105 - 32].height() );
            font[237 - 32].reset();
            fheroes2::Copy( font[105 - 32], 0, 3, font[237 - 32], 0, 3, font[105 - 32].width(), font[105 - 32].height() );
            // Remove old dot shadow
            fheroes2::FillTransform( font[237 - 32], 0, 3, 1, 1, 1 );
            // Add acute accent
            fheroes2::Copy( font[140 - 32], 4, 0, font[237 - 32], 2, 0, 3, 2 );
            font[237 - 32].setPosition( font[105 - 32].x(), font[105 - 32].y() );
            updateNormalFontLetterShadow( font[237 - 32] );

            // Lowercase d with caron. Requires acute accent.
            font[239 - 32].resize( font[100 - 32].width() + 3, font[100 - 32].height() );
            font[239 - 32].reset();
            fheroes2::Copy( font[100 - 32], 0, 0, font[239 - 32], 0, 0, font[100 - 32].width(), font[100 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[239 - 32], 10, 0, 3, 2 );
            font[239 - 32].setPosition( font[100 - 32].x(), font[100 - 32].y() );
            updateNormalFontLetterShadow( font[239 - 32] );

            // Lowercase n with acute
            font[241 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 3 );
            font[241 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[241 - 32], 0, 3, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[241 - 32], 4, 0, 3, 2 );
            font[241 - 32].setPosition( font[110 - 32].x(), font[110 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[241 - 32] );

            // Lowercase n with caron
            font[242 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 3 );
            font[242 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[242 - 32], 0, 3, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[242 - 32], 4, 0, 3, 2 );
            font[242 - 32].setPosition( font[110 - 32].x(), font[110 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[242 - 32] );

            // Lowercase o with acute
            font[243 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[243 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[243 - 32], 4, 0, 3, 2 );
            font[243 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[243 - 32] );

            // Lowercase o with circumflex
            font[244 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[244 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[244 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[65], 1, 1, font[244 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[244 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[244 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 1, font[244 - 32], 6, 1, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[244 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[244 - 32], 6, 0, 1, 1 );
            font[244 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[244 - 32] );

            // Lowercase o with double acute
            font[245 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[245 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[245 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[245 - 32], 2, 0, 3, 2 );
            fheroes2::Copy( font[140 - 32], 4, 0, font[245 - 32], 6, 0, 3, 2 );
            font[245 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[245 - 32] );

            // Lowercase o with diaeresis
            font[246 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[246 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[246 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[246 - 32], 2, 0, 2, 2 );
            fheroes2::Copy( font[175 - 32], 5, 0, font[246 - 32], 6, 0, 2, 2 );
            font[246 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[246 - 32] );

            // Lowercase r with caron
            font[248 - 32].resize( font[114 - 32].width(), font[114 - 32].height() + 3 );
            font[248 - 32].reset();
            fheroes2::Copy( font[114 - 32], 0, 0, font[248 - 32], 0, 3, font[114 - 32].width(), font[114 - 32].height() );
            fheroes2::Copy( font[138 - 32], 4, 0, font[248 - 32], 4, 0, 3, 2 );
            font[248 - 32].setPosition( font[114 - 32].x(), font[114 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[248 - 32] );

            // Lowercase u with ring above
            font[249 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 4 );
            font[249 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[249 - 32], 0, 4, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[217 - 32], 5, 0, font[249 - 32], 3, 0, 1, 3 );
            fheroes2::Copy( font[217 - 32], 8, 0, font[249 - 32], 7, 0, 1, 3 );
            fheroes2::Copy( font[65], 2, 0, font[249 - 32], 4, 0, 3, 1 );
            fheroes2::Copy( font[65], 2, 0, font[249 - 32], 4, 2, 3, 1 );
            fheroes2::Copy( font[69], 3, 2, font[249 - 32], 4, 1, 3, 1 );
            font[249 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[249 - 32] );

            // Lowercase u with acute
            font[250 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 3 );
            font[250 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[250 - 32], 0, 3, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[250 - 32], 4, 0, 3, 2 );
            font[250 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[250 - 32] );

            // Lowercase u with double acute
            font[251 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 3 );
            font[251 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[251 - 32], 0, 3, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[251 - 32], 2, 0, 3, 2 );
            fheroes2::Copy( font[140 - 32], 4, 0, font[251 - 32], 6, 0, 3, 2 );
            font[251 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[251 - 32] );

            // Lowercase u with diaeresis
            font[252 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 3 );
            font[252 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[252 - 32], 0, 3, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[252 - 32], 2, 0, 2, 2 );
            fheroes2::Copy( font[175 - 32], 5, 0, font[252 - 32], 6, 0, 2, 2 );
            font[252 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[252 - 32] );

            // Lowercase y with acute
            font[253 - 32].resize( font[121 - 32].width(), font[121 - 32].height() + 3 );
            font[253 - 32].reset();
            fheroes2::Copy( font[121 - 32], 0, 0, font[253 - 32], 0, 3, font[121 - 32].width(), font[121 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[253 - 32], 5, 0, 3, 2 );
            font[253 - 32].setPosition( font[121 - 32].x(), font[121 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[253 - 32] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // Uppercase S with caron
            font[138 - 32].resize( font[83 - 32].width(), font[83 - 32].height() + 3 );
            font[138 - 32].reset();
            fheroes2::Copy( font[83 - 32], 0, 0, font[138 - 32], 0, 3, font[83 - 32].width(), font[83 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[138 - 32], 3, 0, 3, 2 );
            font[138 - 32].setPosition( font[83 - 32].x(), font[83 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[138 - 32] );

            // Uppercase S with acute
            font[140 - 32].resize( font[83 - 32].width(), font[83 - 32].height() + 3 );
            font[140 - 32].reset();
            fheroes2::Copy( font[83 - 32], 0, 0, font[140 - 32], 0, 3, font[83 - 32].width(), font[83 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[140 - 32], 4, 0, 2, 2 );
            font[140 - 32].setPosition( font[83 - 32].x(), font[83 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[140 - 32] );

            // Uppercase T with caron
            font[141 - 32].resize( font[84 - 32].width(), font[84 - 32].height() + 3 );
            font[141 - 32].reset();
            fheroes2::Copy( font[84 - 32], 0, 0, font[141 - 32], 0, 3, font[84 - 32].width(), font[84 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[141 - 32], 3, 0, 3, 2 );
            font[141 - 32].setPosition( font[84 - 32].x(), font[84 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[141 - 32] );

            // Uppercase Z with caron
            font[142 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[142 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[142 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[142 - 32], 3, 0, 3, 2 );
            font[142 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[142 - 32] );

            // Uppercase Z with acute
            font[143 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[143 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[143 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[143 - 32], 4, 0, 2, 2 );
            font[143 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[143 - 32] );

            // Lowercase s with caron
            font[154 - 32].resize( font[115 - 32].width(), font[115 - 32].height() + 3 );
            font[154 - 32].reset();
            fheroes2::Copy( font[115 - 32], 0, 0, font[154 - 32], 0, 3, font[115 - 32].width(), font[115 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[154 - 32], 2, 0, 3, 2 );
            font[154 - 32].setPosition( font[115 - 32].x(), font[115 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[154 - 32] );

            // Lowercase s with acute
            font[156 - 32].resize( font[115 - 32].width(), font[115 - 32].height() + 3 );
            font[156 - 32].reset();
            fheroes2::Copy( font[115 - 32], 0, 0, font[156 - 32], 0, 3, font[115 - 32].width(), font[115 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[156 - 32], 3, 0, 2, 2 );
            font[156 - 32].setPosition( font[115 - 32].x(), font[115 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[156 - 32] );

            // Lowercase t with caron
            font[157 - 32].resize( font[116 - 32].width(), font[116 - 32].height() + 1 );
            font[157 - 32].reset();
            fheroes2::Copy( font[116 - 32], 0, 0, font[157 - 32], 0, 1, font[116 - 32].width(), font[116 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[157 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[157 - 32], 4, 1, 1, 1 );
            font[157 - 32].setPosition( font[116 - 32].x(), font[116 - 32].y() - 1 );
            updateSmallFontLetterShadow( font[157 - 32] );

            // Lowercase z with caron
            font[158 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 3 );
            font[158 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[158 - 32], 0, 3, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[158 - 32], 2, 0, 3, 2 );
            font[158 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[158 - 32] );

            // Lowercase z with acute
            font[159 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 3 );
            font[159 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[159 - 32], 0, 3, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[159 - 32], 3, 0, 2, 2 );
            font[159 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[159 - 32] );

            // Uppercase L with stroke
            font[163 - 32].resize( font[76 - 32].width(), font[76 - 32].height() );
            font[163 - 32].reset();
            fheroes2::Copy( font[76 - 32], 0, 0, font[163 - 32], 0, 0, font[76 - 32].width(), font[76 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[163 - 32], 4, 2, 2, 2 );
            font[163 - 32].setPosition( font[76 - 32].x(), font[76 - 32].y() );
            updateSmallFontLetterShadow( font[163 - 32] );

            // Uppercase A with ogonek
            font[165 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 2 );
            font[165 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[165 - 32], 0, 0, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[65 - 32], 7, 5, font[165 - 32], 7, 7, 2, 2 );
            font[165 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() );
            updateSmallFontLetterShadow( font[165 - 32] );

            // Uppercase Z with dot above
            font[175 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 2 );
            font[175 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[175 - 32], 0, 2, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[90 - 32], 2, 0, font[175 - 32], 3, 0, 2, 1 );
            font[175 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[175 - 32] );

            // Lowercase l with stroke
            font[179 - 32].resize( font[108 - 32].width(), font[108 - 32].height() );
            font[179 - 32].reset();
            fheroes2::Copy( font[108 - 32], 0, 0, font[179 - 32], 0, 0, font[108 - 32].width(), font[108 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 3, font[179 - 32], 3, 2, 1, 1 );
            fheroes2::Copy( font[122 - 32], 2, 3, font[179 - 32], 1, 4, 1, 1 );
            font[179 - 32].setPosition( font[108 - 32].x(), font[108 - 32].y() );
            updateSmallFontLetterShadow( font[179 - 32] );

            // Lowercase a with ogonek
            font[185 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 2 );
            font[185 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[185 - 32], 0, 0, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[65 - 32], 7, 5, font[185 - 32], 5, 5, 2, 2 );
            font[185 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() );
            updateSmallFontLetterShadow( font[185 - 32] );

            // Uppercase L with caron (NOT an uppercase Y with diaeresis)
            font[188 - 32].resize( font[76 - 32].width(), font[76 - 32].height() );
            font[188 - 32].reset();
            fheroes2::Copy( font[76 - 32], 0, 0, font[188 - 32], 0, 0, font[76 - 32].width(), font[76 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[188 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[188 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[188 - 32], 6, 1, 1, 1 );
            font[188 - 32].setPosition( font[76 - 32].x(), font[76 - 32].y() );
            updateSmallFontLetterShadow( font[188 - 32] );

            // Lowercase l with caron (NOT an uppercase L with caron)
            font[190 - 32].resize( font[108 - 32].width() + 1, font[108 - 32].height() );
            font[190 - 32].reset();
            fheroes2::Copy( font[108 - 32], 0, 0, font[190 - 32], 0, 0, font[108 - 32].width(), font[108 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[190 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[190 - 32], 4, 1, 1, 1 );
            font[190 - 32].setPosition( font[108 - 32].x(), font[108 - 32].y() );
            updateSmallFontLetterShadow( font[190 - 32] );

            // Lowercase z with dot above
            font[191 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 2 );
            font[191 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[191 - 32], 0, 2, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[90 - 32], 2, 0, font[191 - 32], 3, 0, 2, 1 );
            font[191 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[191 - 32] );

            // Uppercase R with acute
            font[192 - 32].resize( font[82 - 32].width(), font[82 - 32].height() + 3 );
            font[192 - 32].reset();
            fheroes2::Copy( font[82 - 32], 0, 0, font[192 - 32], 0, 3, font[82 - 32].width(), font[82 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[192 - 32], 5, 0, 2, 2 );
            font[192 - 32].setPosition( font[82 - 32].x(), font[82 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[192 - 32] );

            // Uppercase A with acute
            font[193 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[193 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[193 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[193 - 32], 5, 0, 2, 2 );
            font[193 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[193 - 32] );

            // Uppercase A with circumflex
            font[194 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[194 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 6, 1, 1, 1 );
            font[194 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateSmallFontLetterShadow( font[194 - 32] );

            // Uppercase A with diaeresis
            font[196 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 2 );
            font[196 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[196 - 32], 0, 2, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[196 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[196 - 32], 6, 0, 1, 1 );
            font[196 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[196 - 32] );

            // Uppercase L with acute
            font[197 - 32].resize( font[76 - 32].width(), font[76 - 32].height() + 3 );
            font[197 - 32].reset();
            fheroes2::Copy( font[76 - 32], 0, 0, font[197 - 32], 0, 3, font[76 - 32].width(), font[76 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[197 - 32], 5, 0, 2, 2 );
            font[197 - 32].setPosition( font[76 - 32].x(), font[76 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[197 - 32] );

            // Uppercase C with acute
            font[198 - 32].resize( font[67 - 32].width(), font[67 - 32].height() + 3 );
            font[198 - 32].reset();
            fheroes2::Copy( font[67 - 32], 0, 0, font[198 - 32], 0, 3, font[67 - 32].width(), font[67 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[198 - 32], 4, 0, 2, 2 );
            font[198 - 32].setPosition( font[67 - 32].x(), font[67 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[198 - 32] );

            // Uppercase C with caron
            font[200 - 32].resize( font[67 - 32].width(), font[67 - 32].height() + 3 );
            font[200 - 32].reset();
            fheroes2::Copy( font[67 - 32], 0, 0, font[200 - 32], 0, 3, font[67 - 32].width(), font[67 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[200 - 32], 3, 0, 3, 2 );
            font[200 - 32].setPosition( font[67 - 32].x(), font[67 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[200 - 32] );

            // Uppercase E with acute
            font[201 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[201 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[201 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[201 - 32], 4, 0, 2, 2 );
            font[201 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[201 - 32] );

            // Uppercase E with ogonek
            font[202 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 2 );
            font[202 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[202 - 32], 0, 0, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[65 - 32], 7, 5, font[202 - 32], 5, 7, 2, 2 );
            font[202 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() );
            updateSmallFontLetterShadow( font[202 - 32] );

            // Uppercase E with caron
            font[204 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[204 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[204 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[204 - 32], 3, 0, 3, 2 );
            font[204 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[204 - 32] );

            // Uppercase I with acute
            font[205 - 32].resize( font[73 - 32].width(), font[73 - 32].height() + 3 );
            font[205 - 32].reset();
            fheroes2::Copy( font[73 - 32], 0, 0, font[205 - 32], 0, 3, font[73 - 32].width(), font[73 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[205 - 32], 2, 0, 2, 2 );
            font[205 - 32].setPosition( font[73 - 32].x(), font[73 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[205 - 32] );

            // Uppercase D with caron
            font[207 - 32].resize( font[68 - 32].width(), font[68 - 32].height() + 3 );
            font[207 - 32].reset();
            fheroes2::Copy( font[68 - 32], 0, 0, font[207 - 32], 0, 3, font[68 - 32].width(), font[68 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[207 - 32], 3, 0, 3, 2 );
            font[207 - 32].setPosition( font[68 - 32].x(), font[68 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[207 - 32] );

            // Uppercase N with acute
            font[209 - 32].resize( font[78 - 32].width(), font[78 - 32].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[78 - 32], 0, 0, font[209 - 32], 0, 3, font[78 - 32].width(), font[78 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[209 - 32], 5, 0, 2, 2 );
            font[209 - 32].setPosition( font[78 - 32].x(), font[78 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[209 - 32] );

            // Uppercase N with caron
            font[210 - 32].resize( font[78 - 32].width(), font[78 - 32].height() + 3 );
            font[210 - 32].reset();
            fheroes2::Copy( font[78 - 32], 0, 0, font[210 - 32], 0, 3, font[78 - 32].width(), font[78 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[210 - 32], 5, 0, 3, 2 );
            font[210 - 32].setPosition( font[78 - 32].x(), font[78 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[210 - 32] );

            // Uppercase O with acute
            font[211 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[211 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[211 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[211 - 32], 4, 0, 2, 2 );
            font[211 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[211 - 32] );

            // Uppercase O with double circumflex
            font[212 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[212 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[212 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[212 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[212 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[212 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[212 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[212 - 32], 6, 1, 1, 1 );
            font[212 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[212 - 32] );

            // Uppercase O with double acute
            font[213 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[213 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[213 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[213 - 32], 2, 0, 2, 2 );
            fheroes2::Copy( font[122 - 32], 2, 2, font[213 - 32], 5, 0, 2, 2 );
            font[213 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[213 - 32] );

            // Uppercase O with diaeresis
            font[214 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 2 );
            font[214 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[214 - 32], 0, 2, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[122 - 32], 3, 2, font[214 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[122 - 32], 3, 2, font[214 - 32], 6, 0, 1, 1 );
            font[214 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[214 - 32] );

            // Uppercase R with caron
            font[216 - 32].resize( font[82 - 32].width(), font[82 - 32].height() + 3 );
            font[216 - 32].reset();
            fheroes2::Copy( font[82 - 32], 0, 0, font[216 - 32], 0, 3, font[82 - 32].width(), font[82 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[216 - 32], 4, 0, 3, 2 );
            font[216 - 32].setPosition( font[82 - 32].x(), font[82 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[216 - 32] );

            // Uppercase U with ring above
            font[217 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[217 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[217 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[217 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[116 - 32], 2, 5, font[217 - 32], 4, 1, 3, 2 );
            font[217 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[217 - 32] );

            // Uppercase U with acute
            font[218 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[218 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[218 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[218 - 32], 5, 0, 2, 2 );
            font[218 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[218 - 32] );

            // Uppercase U with double acute
            font[219 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[219 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[219 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[219 - 32], 3, 0, 2, 2 );
            fheroes2::Copy( font[122 - 32], 2, 2, font[219 - 32], 7, 0, 2, 2 );
            font[219 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[219 - 32] );

            // Uppercase U with diaeresis
            font[220 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 2 );
            font[220 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[220 - 32], 0, 2, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[122 - 32], 3, 2, font[220 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[122 - 32], 3, 2, font[220 - 32], 7, 0, 1, 1 );
            font[220 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[220 - 32] );

            // Uppercase Y with acute
            font[221 - 32].resize( font[89 - 32].width(), font[89 - 32].height() + 3 );
            font[221 - 32].reset();
            fheroes2::Copy( font[89 - 32], 0, 0, font[221 - 32], 0, 3, font[89 - 32].width(), font[89 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[221 - 32], 5, 0, 2, 2 );
            font[221 - 32].setPosition( font[89 - 32].x(), font[89 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[221 - 32] );

            // Lowercase r with acute
            font[224 - 32].resize( font[114 - 32].width(), font[114 - 32].height() + 3 );
            font[224 - 32].reset();
            fheroes2::Copy( font[114 - 32], 0, 0, font[224 - 32], 0, 3, font[114 - 32].width(), font[114 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[224 - 32], 3, 0, 2, 2 );
            font[224 - 32].setPosition( font[114 - 32].x(), font[114 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[224 - 32] );

            // Lowercase a with acute
            font[225 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[225 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[225 - 32], 3, 0, 2, 2 );
            font[225 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[225 - 32] );

            // Lowercase a with circumflex
            font[226 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[226 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[226 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[226 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[226 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[226 - 32], 5, 1, 1, 1 );
            font[226 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[226 - 32] );

            // Lowercase a with diaeresis
            font[228 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 2 );
            font[228 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[228 - 32], 0, 2, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[228 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[228 - 32], 5, 0, 1, 1 );
            font[228 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[228 - 32] );

            // Lowercase l with acute
            font[229 - 32].resize( font[108 - 32].width(), font[108 - 32].height() + 3 );
            font[229 - 32].reset();
            fheroes2::Copy( font[108 - 32], 0, 0, font[229 - 32], 0, 3, font[108 - 32].width(), font[108 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[229 - 32], 2, 0, 2, 2 );
            font[229 - 32].setPosition( font[108 - 32].x(), font[108 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[229 - 32] );

            // Lowercase c with acute
            font[230 - 32].resize( font[99 - 32].width(), font[99 - 32].height() + 3 );
            font[230 - 32].reset();
            fheroes2::Copy( font[99 - 32], 0, 0, font[230 - 32], 0, 3, font[99 - 32].width(), font[99 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[230 - 32], 3, 0, 2, 2 );
            font[230 - 32].setPosition( font[99 - 32].x(), font[99 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[230 - 32] );

            // Lowercase c with caron
            font[232 - 32].resize( font[99 - 32].width(), font[99 - 32].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[99 - 32], 0, 0, font[232 - 32], 0, 3, font[99 - 32].width(), font[99 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[232 - 32], 2, 0, 3, 2 );
            font[232 - 32].setPosition( font[99 - 32].x(), font[99 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[232 - 32] );

            // Lowercase e with acute
            font[233 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[233 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[233 - 32], 3, 0, 2, 2 );
            font[233 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[233 - 32] );

            // Lowercase e with ogonek
            font[234 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 2 );
            font[234 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[234 - 32], 0, 0, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[65 - 32], 7, 5, font[234 - 32], 3, 5, 2, 2 );
            font[234 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() );
            updateSmallFontLetterShadow( font[234 - 32] );

            // Lowercase e with caron
            font[236 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[236 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[236 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[236 - 32], 2, 0, 3, 2 );
            font[236 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[236 - 32] );

            // Lowercase i with acute
            font[237 - 32].resize( font[105 - 32].width(), font[105 - 32].height() + 1 );
            font[237 - 32].reset();
            fheroes2::Copy( font[105 - 32], 0, 0, font[237 - 32], 0, 1, font[105 - 32].width(), font[105 - 32].height() );
            fheroes2::Copy( font[122 - 32], 3, 2, font[237 - 32], 3, 0, 1, 1 );
            font[237 - 32].setPosition( font[105 - 32].x(), font[105 - 32].y() - 1 );
            updateSmallFontLetterShadow( font[237 - 32] );

            // Lowercase d with caron
            font[239 - 32].resize( font[100 - 32].width() + 2, font[100 - 32].height() );
            font[239 - 32].reset();
            fheroes2::Copy( font[100 - 32], 0, 0, font[239 - 32], 0, 0, font[100 - 32].width(), font[100 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[239 - 32], 7, 0, 2, 2 );
            font[239 - 32].setPosition( font[100 - 32].x(), font[100 - 32].y() );
            updateSmallFontLetterShadow( font[239 - 32] );

            // Lowercase n with acute
            font[241 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 3 );
            font[241 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[241 - 32], 0, 3, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[241 - 32], 4, 0, 2, 2 );
            font[241 - 32].setPosition( font[110 - 32].x(), font[110 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[241 - 32] );

            // Lowercase n with caron
            font[242 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 3 );
            font[242 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[242 - 32], 0, 3, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[242 - 32], 3, 0, 3, 2 );
            font[242 - 32].setPosition( font[110 - 32].x(), font[110 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[242 - 32] );

            // Lowercase o with acute
            font[243 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[243 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[243 - 32], 3, 0, 2, 2 );
            font[243 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[243 - 32] );

            // Lowercase o with circumflex
            font[244 - 32].resize( font[111 - 32].width() + 1, font[111 - 32].height() + 3 );
            font[244 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[244 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[97 - 32], 2, 0, font[244 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[244 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 2, 0, font[244 - 32], 4, 1, 1, 1 );
            font[244 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[244 - 32] );

            // Lowercase o with double acute
            font[245 - 32].resize( font[111 - 32].width() + 1, font[111 - 32].height() + 3 );
            font[245 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[245 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[245 - 32], 2, 0, 2, 2 );
            fheroes2::Copy( font[122 - 32], 2, 2, font[245 - 32], 5, 0, 2, 2 );
            font[245 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[245 - 32] );

            // Lowercase o with diaeresis
            font[246 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 2 );
            font[246 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[246 - 32], 0, 2, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[122 - 32], 3, 2, font[246 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[122 - 32], 3, 2, font[246 - 32], 4, 0, 1, 1 );
            font[246 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[246 - 32] );

            // Lowercase r with caron
            font[248 - 32].resize( font[114 - 32].width(), font[114 - 32].height() + 3 );
            font[248 - 32].reset();
            fheroes2::Copy( font[114 - 32], 0, 0, font[248 - 32], 0, 3, font[114 - 32].width(), font[114 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[248 - 32], 3, 0, 3, 2 );
            font[248 - 32].setPosition( font[114 - 32].x(), font[114 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[248 - 32] );

            // Lowercase u with ring above
            font[249 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 4 );
            font[249 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[249 - 32], 0, 4, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[116 - 32], 2, 5, font[249 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[116 - 32], 2, 5, font[249 - 32], 3, 1, 3, 2 );
            font[249 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 4 );
            updateSmallFontLetterShadow( font[249 - 32] );

            // Lowercase u with acute
            font[250 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 3 );
            font[250 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[250 - 32], 0, 3, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[250 - 32], 3, 0, 2, 2 );
            font[250 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[250 - 32] );

            // Lowercase u with double acute
            font[251 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 3 );
            font[251 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[251 - 32], 0, 3, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[251 - 32], 2, 0, 2, 2 );
            fheroes2::Copy( font[122 - 32], 2, 2, font[251 - 32], 5, 0, 2, 2 );
            font[251 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[251 - 32] );

            // Lowercase u with diaeresis
            font[252 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 2 );
            font[252 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[252 - 32], 0, 2, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[122 - 32], 3, 2, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[122 - 32], 3, 2, font[252 - 32], 6, 0, 1, 1 );
            font[252 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[252 - 32] );

            // Lowercase y with acute
            font[253 - 32].resize( font[121 - 32].width(), font[121 - 32].height() + 3 );
            font[253 - 32].reset();
            fheroes2::Copy( font[121 - 32], 0, 0, font[253 - 32], 0, 3, font[121 - 32].width(), font[121 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[253 - 32], 4, 0, 2, 2 );
            font[253 - 32].setPosition( font[121 - 32].x(), font[121 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[253 - 32] );
        }
    }

    void generateFrenchAlphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // Lowercase o with circumflex
            font[35 - 32] = font[244 - 32];
            // Lowercase u with circumflex
            // Lowercase u with circumflex
            font[36 - 32] = font[251 - 32];
            // Lowercase u with grave accent
            font[38 - 32] = font[249 - 32];
            // Lowercase a with circumflex
            font[42 - 32] = font[226 - 32];
            // Lowercase i with diaeresis
            font[60 - 32] = font[239 - 32];
            // Lowercase i with circumflex <- Confirmed used in the OG Succession wars.
            font[62 - 32] = font[238 - 32];
            // Lowercase a with grave accent
            font[64 - 32] = font[224 - 32];
            // Lowercase c with cedilla
            font[94 - 32] = font[231 - 32];
            // Lowercase e with grave accent
            font[96 - 32] = font[232 - 32];
            // Lowercase i with diaeresis
            font[123 - 32] = font[239 - 32];
            // Lowercase e with circumflex
            font[124 - 32] = font[234 - 32];
            // Lowercase i with circumflex
            font[125 - 32] = font[239 - 32];
            // Lowercase e with acute
            font[126 - 32] = font[233 - 32];
            // Lowercase i with circumflex
            font[127 - 32] = font[239 - 32];
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // Lowercase o with circumflex
            font[35 - 32] = font[244 - 32];
            // Lowercase u with circumflex
            font[36 - 32] = font[251 - 32];
            // Lowercase u with grave accent
            font[38 - 32] = font[249 - 32];
            // Lowercase a with circumflex
            font[42 - 32] = font[226 - 32];
            // Lowercase i with diaeresis
            font[60 - 32] = font[239 - 32];
            // Lowercase i with circumflex
            font[62 - 32] = font[238 - 32];
            // Lowercase a with grave accent
            font[64 - 32] = font[224 - 32];
            // Lowercase c with cedilla
            font[94 - 32] = font[231 - 32];
            // Lowercase e with grave accent
            font[96 - 32] = font[232 - 32];
            // Lowercase i with diaeresis
            font[123 - 32] = font[239 - 32];
            // Lowercase e with circumflex
            font[124 - 32] = font[234 - 32];
            // Lowercase i with circumflex
            font[125 - 32] = font[239 - 32];
            // Lowercase e with acute
            font[126 - 32] = font[233 - 32];
            // Lowercase i with circumflex
            font[127 - 32] = font[239 - 32];
        }
    }

    // CP-1251 supports Russian, Ukrainian, Belarussian, Bulgarian, Serbian Cyrillic, Macedonian and English.
    void generateCP1251Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // TODO: add support for Serbian Cyrillic and Macedonian languages by generating missing letters.

        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = icnVsSprite[icnId];

            original.resize( baseFontSize );

            const fheroes2::Sprite firstSprite{ original[0] };
            original.insert( original.end(), 128, firstSprite );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            size_t offset = 0;

            // ' (right single quotation mark)
            font[146 - 32] = font[44 - 32];
            font[146 - 32].setPosition( font[146 - 32].x(), font[146 - 32].y() - 6 );

            // E with 2 dots on top.
            font[168 - 32].resize( font[37 + offset].width(), font[37 + offset].height() + 3 );
            font[168 - 32].reset();
            fheroes2::Copy( font[37 + offset], 0, 0, font[168 - 32], 0, 3, font[37 + offset].width(), font[37 + offset].height() );
            fheroes2::Copy( font[168 - 32], 5, 5, font[168 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[168 - 32], 5, 5, font[168 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[168 - 32], 4, 5, font[168 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[168 - 32], 4, 5, font[168 - 32], 7, 1, 1, 1 );
            font[168 - 32].setPosition( font[37 + offset].x(), font[37 + offset].y() - 3 );
            updateNormalFontLetterShadow( font[168 - 32] );

            font[161 - 32].resize( font[57 + offset].width(), font[57 + offset].height() + 3 );
            font[161 - 32].reset();
            fheroes2::Copy( font[57 + offset], 0, 0, font[161 - 32], 0, 3, font[57 + offset].width(), font[57 + offset].height() );
            fheroes2::Copy( font[168 - 32], 3, 0, font[161 - 32], 7, 0, 2, 3 );
            font[161 - 32].setPosition( font[57 + offset].x(), font[57 + offset].y() - 3 );
            updateNormalFontLetterShadow( font[161 - 32] );

            font[162 - 32].resize( font[89 + offset].width(), font[89 + offset].height() + 3 );
            font[162 - 32].reset();
            fheroes2::Copy( font[89 + offset], 0, 0, font[162 - 32], 0, 3, font[89 + offset].width(), font[89 + offset].height() );
            fheroes2::Copy( font[89 + offset], 4, 1, font[162 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[89 + offset], 4, 0, font[162 - 32], 6, 1, 1, 1 );
            font[162 - 32].setPosition( font[89 + offset].x(), font[89 + offset].y() - 3 );
            updateNormalFontLetterShadow( font[162 - 32] );

            // C with a horizontal line in the middle.
            font[170 - 32] = font[67 - 32];
            fheroes2::Copy( font[170 - 32], 7, 0, font[170 - 32], 6, 5, 4, 2 );
            updateNormalFontLetterShadow( font[170 - 32] );

            font[186 - 32] = font[99 - 32];
            fheroes2::Copy( font[186 - 32], 4, 0, font[186 - 32], 3, 3, 3, 1 );
            updateNormalFontLetterShadow( font[186 - 32] );

            // I and i with 2 dots.
            font[175 - 32].resize( font[73 - 32].width(), font[73 - 32].height() + 3 );
            font[175 - 32].reset();
            fheroes2::Copy( font[73 - 32], 0, 0, font[175 - 32], 0, 3, font[73 - 32].width(), font[73 - 32].height() );
            fheroes2::Copy( font[168 - 32], 3, 0, font[175 - 32], 2, 0, 5, 3 );
            font[175 - 32].setPosition( font[73 - 32].x(), font[73 - 32].y() - 3 );

            font[191 - 32] = font[105 - 32];
            fheroes2::FillTransform( font[191 - 32], 2, 0, 1, 3, 1 );

            // J and j.
            font[163 - 32] = font[74 - 32];
            font[188 - 32] = font[106 - 32];

            // S and s.
            font[189 - 32] = font[83 - 32];
            font[190 - 32] = font[115 - 32];

            // I and i.
            font[178 - 32] = font[73 - 32];
            font[179 - 32] = font[105 - 32];

            // A
            font[192 - 32] = font[33];

            font[193 - 32] = font[34 + offset];
            fheroes2::FillTransform( font[193 - 32], 9, 4, 2, 1, 1 );
            fheroes2::Copy( font[38], 6, 0, font[193 - 32], 6, 0, 5, 4 );
            fheroes2::Copy( font[193 - 32], 9, 5, font[193 - 32], 8, 4, 1, 1 );
            updateNormalFontLetterShadow( font[193 - 32] );

            font[194 - 32] = font[34 + offset];

            font[195 - 32] = font[38];
            fheroes2::FillTransform( font[195 - 32], 6, 4, 3, 4, 1 );

            // The same letter as above but with a vertical line at the top.
            font[165 - 32].resize( font[195 - 32].width(), font[195 - 32].height() + 1 );
            font[165 - 32].reset();
            fheroes2::Copy( font[195 - 32], 0, 0, font[165 - 32], 0, 1, font[195 - 32].width(), font[195 - 32].height() );
            fheroes2::Copy( font[195 - 32], 9, 1, font[165 - 32], 9, 0, 2, 1 );
            fheroes2::Copy( font[195 - 32], 9, 1, font[165 - 32], 10, 1, 1, 1 );
            fheroes2::Copy( font[195 - 32], 10, 0, font[165 - 32], 10, 2, 1, 1 );
            fheroes2::Copy( font[195 - 32], 8, 1, font[165 - 32], 9, 2, 1, 1 );
            font[165 - 32].setPosition( font[195 - 32].x(), font[195 - 32].y() - 1 );

            font[196 - 32] = font[36 + offset];

            font[197 - 32] = font[37 + offset];

            // x with | in the middle.
            font[198 - 32].resize( font[56].width() + 1, font[56].height() );
            font[198 - 32].reset();
            fheroes2::Copy( font[56], 1, 0, font[198 - 32], 1, 0, 8, 11 );
            fheroes2::Copy( font[56], 9, 0, font[198 - 32], 10, 0, 6, 11 );
            fheroes2::Fill( font[198 - 32], 9, 1, 1, 9, font[198 - 32].image()[1 + font[198 - 32].width()] );
            font[198 - 32].setPosition( font[56].x(), font[56].y() );
            updateNormalFontLetterShadow( font[198 - 32] );

            font[199 - 32].resize( font[19].width() + 1, font[19].height() );
            font[199 - 32].reset();
            fheroes2::Copy( font[19], 1, 0, font[199 - 32], 1, 0, 5, 3 );
            fheroes2::Copy( font[19], 5, 0, font[199 - 32], 6, 0, 3, 4 );
            fheroes2::Copy( font[19], 3, 5, font[199 - 32], 4, 4, 5, 4 );
            fheroes2::Copy( font[19], 1, 8, font[199 - 32], 1, 8, 5, 3 );
            fheroes2::Copy( font[19], 5, 8, font[199 - 32], 6, 8, 3, 3 );
            fheroes2::FillTransform( font[199 - 32], 2, 6, 5, 3, 1 );
            font[199 - 32].setPosition( font[19].x(), font[19].y() );
            updateNormalFontLetterShadow( font[199 - 32] );

            // Reverted N.
            font[200 - 32] = font[46];
            fheroes2::FillTransform( font[200 - 32], 6, 1, 5, 11, 1 );
            fheroes2::Copy( font[46], 6, 2, font[200 - 32], 6, 6, 1, 3 );
            fheroes2::Copy( font[46], 7, 3, font[200 - 32], 7, 5, 1, 3 );
            fheroes2::Copy( font[46], 8, 4, font[200 - 32], 8, 4, 1, 3 );
            fheroes2::Copy( font[46], 8, 4, font[200 - 32], 9, 3, 1, 3 );
            fheroes2::Copy( font[46], 8, 4, font[200 - 32], 10, 2, 1, 3 );
            fheroes2::Copy( font[46], 8, 4, font[200 - 32], 11, 1, 1, 3 );
            fheroes2::Copy( font[46], 11, 7, font[200 - 32], 11, 8, 1, 1 );
            fheroes2::Copy( font[46], 13, 9, font[200 - 32], 11, 9, 1, 1 );
            updateNormalFontLetterShadow( font[200 - 32] );

            font[201 - 32].resize( font[200 - 32].width(), font[200 - 32].height() + 3 );
            font[201 - 32].reset();
            fheroes2::Copy( font[200 - 32], 0, 0, font[201 - 32], 0, 3, font[200 - 32].width(), font[200 - 32].height() );
            font[201 - 32].setPosition( font[200 - 32].x(), font[200 - 32].y() - 3 );
            fheroes2::Copy( font[201 - 32], 12, 4, font[201 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[201 - 32], 11, 10, font[201 - 32], 8, 1, 1, 1 );
            updateNormalFontLetterShadow( font[201 - 32] );

            font[202 - 32] = font[43 + offset];

            font[204 - 32] = font[45 + offset];
            font[205 - 32] = font[40 + offset];
            font[206 - 32] = font[47 + offset];

            font[207 - 32] = font[195 - 32];
            fheroes2::Copy( font[207 - 32], 4, 1, font[207 - 32], 8, 1, 2, 9 );
            fheroes2::Copy( font[207 - 32], 4, 9, font[207 - 32], 8, 10, 2, 1 );
            fheroes2::Copy( font[207 - 32], 6, 0, font[207 - 32], 10, 0, 1, 2 );
            updateNormalFontLetterShadow( font[207 - 32] );

            font[203 - 32].resize( font[207 - 32].width() - 1, font[207 - 32].height() );
            font[203 - 32].reset();
            fheroes2::Copy( font[207 - 32], 0, 0, font[203 - 32], 0, 0, font[207 - 32].width() - 1, font[207 - 32].height() );
            fheroes2::FillTransform( font[203 - 32], 0, 0, 4, 6, 1 );
            fheroes2::FillTransform( font[203 - 32], 4, 0, 3, 2, 1 );
            fheroes2::Copy( font[203 - 32], 4, 2, font[203 - 32], 5, 1, 2, 1 );
            fheroes2::Copy( font[203 - 32], 1, 10, font[203 - 32], 5, 0, 2, 1 );
            font[203 - 32].setPosition( font[207 - 32].x(), font[207 - 32].y() );
            updateNormalFontLetterShadow( font[203 - 32] );

            font[208 - 32] = font[48 + offset];
            font[209 - 32] = font[35 + offset];

            font[210 - 32].resize( font[207 - 32].width() + 4, font[207 - 32].height() );
            font[210 - 32].reset();
            fheroes2::Copy( font[207 - 32], 0, 0, font[210 - 32], 0, 0, font[207 - 32].width(), font[207 - 32].height() );
            fheroes2::Copy( font[210 - 32], 7, 0, font[210 - 32], 11, 0, 4, font[207 - 32].height() );
            font[210 - 32].setPosition( font[207 - 32].x(), font[207 - 32].y() );

            font[211 - 32] = font[57 + offset];

            font[212 - 32].resize( font[48].width() + 1, font[48].height() );
            font[212 - 32].reset();
            fheroes2::Copy( font[48], 0, 0, font[212 - 32], 1, 0, font[48].width(), font[48].height() );
            fheroes2::Flip( font[48], 6, 0, font[212 - 32], 1, 0, 5, 6, true, false );
            font[212 - 32].setPosition( font[48].x(), font[48].y() );
            updateNormalFontLetterShadow( font[212 - 32] );

            font[213 - 32] = font[56 + offset];

            font[214 - 32].resize( font[53].width() + 2, font[53].height() + 1 );
            font[214 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[214 - 32], 0, 0, font[52].width(), font[52].height() );
            fheroes2::Copy( font[214 - 32], 9, 1, font[214 - 32], 11, 9, 1, 1 );
            fheroes2::Copy( font[214 - 32], 9, 1, font[214 - 32], 12, 8, 1, 1 );
            fheroes2::Copy( font[214 - 32], 9, 1, font[214 - 32], 12, 10, 1, 2 );
            fheroes2::Copy( font[214 - 32], 10, 1, font[214 - 32], 12, 9, 1, 1 );
            fheroes2::Copy( font[214 - 32], 10, 1, font[214 - 32], 13, 8, 1, 4 );
            font[214 - 32].setPosition( font[53].x(), font[53].y() );
            updateNormalFontLetterShadow( font[214 - 32] );

            font[216 - 32].resize( font[53].width() + 2, font[53].height() );
            font[216 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[216 - 32], 0, 0, 6, 11 );
            fheroes2::Copy( font[53], 8, 0, font[216 - 32], 7, 0, 3, 11 );
            fheroes2::Copy( font[53], 8, 0, font[216 - 32], 11, 0, 3, 11 );
            fheroes2::Copy( font[204 - 32], 10, 0, font[216 - 32], 6, 5, 3, 5 );
            fheroes2::Copy( font[204 - 32], 10, 0, font[216 - 32], 10, 5, 3, 5 );
            fheroes2::FillTransform( font[216 - 32], 7, 10, 1, 1, 1 );
            fheroes2::FillTransform( font[216 - 32], 11, 10, 1, 1, 1 );
            font[216 - 32].setPosition( font[53].x(), font[53].y() );
            updateNormalFontLetterShadow( font[216 - 32] );

            font[215 - 32] = font[53];
            fheroes2::FillTransform( font[215 - 32], 3, 6, 6, 7, 1 );
            fheroes2::Copy( font[216 - 32], 4, 5, font[215 - 32], 4, 3, 4, 6 );
            fheroes2::Copy( font[215 - 32], 6, 5, font[215 - 32], 8, 3, 1, 3 );
            fheroes2::Copy( font[215 - 32], 7, 4, font[215 - 32], 9, 2, 1, 2 );
            fheroes2::Copy( font[215 - 32], 9, 8, font[215 - 32], 9, 9, 1, 1 );
            updateNormalFontLetterShadow( font[215 - 32] );

            font[217 - 32].resize( font[216 - 32].width() + 2, font[216 - 32].height() + 1 );
            font[217 - 32].reset();
            fheroes2::Copy( font[216 - 32], 0, 0, font[217 - 32], 0, 0, font[216 - 32].width(), font[216 - 32].height() );
            fheroes2::Copy( font[214 - 32], 11, 8, font[217 - 32], 14, 8, 3, 4 );
            font[217 - 32].setPosition( font[216 - 32].x(), font[216 - 32].y() );
            updateNormalFontLetterShadow( font[217 - 32] );

            font[218 - 32].resize( font[193 - 32].width() + 1, font[193 - 32].height() );
            font[218 - 32].reset();
            fheroes2::Copy( font[193 - 32], 0, 0, font[218 - 32], 1, 0, font[193 - 32].width(), font[193 - 32].height() );
            fheroes2::Copy( font[193 - 32], 1, 0, font[218 - 32], 1, 0, 3, 4 );
            fheroes2::FillTransform( font[218 - 32], 7, 0, 5, 4, 1 );
            font[218 - 32].setPosition( font[193 - 32].x(), font[193 - 32].y() );
            updateNormalFontLetterShadow( font[218 - 32] );

            font[220 - 32] = font[193 - 32];
            fheroes2::FillTransform( font[220 - 32], 0, 0, 4, 6, 1 );
            fheroes2::FillTransform( font[220 - 32], 6, 0, 5, 4, 1 );
            fheroes2::Copy( font[53], 8, 0, font[220 - 32], 3, 0, 3, 1 );
            updateNormalFontLetterShadow( font[220 - 32] );

            font[219 - 32].resize( font[220 - 32].width() + 3, font[220 - 32].height() );
            font[219 - 32].reset();
            fheroes2::Copy( font[220 - 32], 0, 0, font[219 - 32], 0, 0, font[220 - 32].width(), font[220 - 32].height() );
            fheroes2::Copy( font[219 - 32], 3, 0, font[219 - 32], 11, 0, 3, 9 );
            fheroes2::Copy( font[207 - 32], 8, 9, font[219 - 32], 12, 9, 2, 2 );
            font[219 - 32].setPosition( font[220 - 32].x(), font[220 - 32].y() );
            updateNormalFontLetterShadow( font[219 - 32] );

            font[221 - 32].resize( font[47].width() - 3, font[47].height() );
            font[221 - 32].reset();
            fheroes2::Copy( font[47], 4, 0, font[221 - 32], 1, 0, 9, 11 );
            fheroes2::FillTransform( font[221 - 32], 0, 3, 3, 5, 1 );
            fheroes2::Copy( font[221 - 32], 3, 0, font[221 - 32], 4, 5, 5, 1 );
            font[221 - 32].setPosition( font[47].x(), font[47].y() );
            updateNormalFontLetterShadow( font[221 - 32] );

            font[222 - 32].resize( font[47].width() + 1, font[47].height() );
            font[222 - 32].reset();
            fheroes2::Copy( font[193 - 32], 0, 0, font[222 - 32], 0, 0, 6, 13 );
            fheroes2::Copy( font[47], 4, 1, font[222 - 32], 7, 1, 4, 8 );
            fheroes2::Copy( font[47], 10, 1, font[222 - 32], 11, 1, 3, 8 );
            fheroes2::Copy( font[47], 5, 0, font[222 - 32], 8, 0, 3, 1 );
            fheroes2::Copy( font[47], 10, 0, font[222 - 32], 11, 0, 2, 1 );
            fheroes2::Copy( font[47], 4, 9, font[222 - 32], 7, 9, 4, 2 );
            fheroes2::Copy( font[47], 10, 9, font[222 - 32], 11, 9, 3, 2 );
            fheroes2::Copy( font[222 - 32], 2, 0, font[222 - 32], 6, 5, 2, 1 );
            font[222 - 32].setPosition( font[193 - 32].x(), font[193 - 32].y() );
            updateNormalFontLetterShadow( font[222 - 32] );

            font[223 - 32].resize( font[203 - 32].width() - 1, font[203 - 32].height() );
            font[223 - 32].reset();
            fheroes2::Copy( font[33], 0, 5, font[223 - 32], 0, 5, 7, 6 );
            fheroes2::Copy( font[212 - 32], 0, 0, font[223 - 32], 1, 0, 7, 6 );
            fheroes2::Copy( font[203 - 32], 8, 0, font[223 - 32], 7, 0, 2, 11 );
            fheroes2::Copy( font[223 - 32], 6, 5, font[223 - 32], 7, 5, 1, 1 );
            font[223 - 32].setPosition( font[203 - 32].x(), font[203 - 32].y() );
            updateNormalFontLetterShadow( font[223 - 32] );

            offset = 32;

            // e with 2 dots on top.
            font[184 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[184 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[184 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[168 - 32], 3, 0, font[184 - 32], 3, 0, 2, 4 );
            fheroes2::Copy( font[168 - 32], 3, 0, font[184 - 32], 5, 0, 2, 4 );
            font[184 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            // Not shadow needs to be updated here.

            font[224 - 32] = font[33 + offset];

            font[225 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[69], 1, 5, font[225 - 32], 1, 8, 8, 2 );
            fheroes2::Copy( font[69], 1, 0, font[225 - 32], 1, 6, 8, 2 );
            fheroes2::Copy( font[67], 1, 0, font[225 - 32], 1, 0, 8, 2 );
            fheroes2::Copy( font[45], 7, 3, font[225 - 32], 1, 2, 3, 1 );
            fheroes2::Copy( font[45], 7, 3, font[225 - 32], 2, 3, 3, 1 );
            fheroes2::Copy( font[45], 7, 3, font[225 - 32], 3, 4, 3, 1 );
            fheroes2::Copy( font[45], 8, 3, font[225 - 32], 6, 5, 2, 1 );
            fheroes2::Copy( font[45], 7, 3, font[225 - 32], 4, 5, 2, 1 );
            font[225 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[225 - 32] );

            font[227 - 32] = font[82];
            fheroes2::Copy( font[227 - 32], 1, 0, font[227 - 32], 3, 0, 2, 1 );
            fheroes2::Copy( font[227 - 32], 4, 2, font[227 - 32], 4, 1, 1, 1 );
            fheroes2::SetTransformPixel( font[227 - 32], 4, 2, 1 );
            updateNormalFontLetterShadow( font[227 - 32] );

            // The same letter as above but with a vertical line at the top.
            font[180 - 32].resize( font[227 - 32].width(), font[227 - 32].height() + 1 );
            font[180 - 32].reset();
            fheroes2::Copy( font[227 - 32], 0, 0, font[180 - 32], 0, 1, font[227 - 32].width(), font[227 - 32].height() );
            fheroes2::Copy( font[227 - 32], 6, 1, font[180 - 32], 6, 0, 3, 1 );
            fheroes2::FillTransform( font[180 - 32], 7, 2, 2, 1, 1 );
            fheroes2::FillTransform( font[180 - 32], 6, 4, 2, 1, 1 );
            font[180 - 32].setPosition( font[227 - 32].x(), font[227 - 32].y() - 1 );

            font[228 - 32] = font[71];
            font[229 - 32] = font[37 + offset];

            // x with | in the middle.
            font[230 - 32].resize( font[88].width() + 2, font[88].height() );
            font[230 - 32].reset();
            fheroes2::Copy( font[88], 0, 0, font[230 - 32], 0, 0, 6, 7 );
            fheroes2::Copy( font[88], 5, 0, font[230 - 32], 7, 0, 5, 7 );
            fheroes2::Fill( font[230 - 32], 6, 1, 1, 5, font[230 - 32].image()[3 + font[230 - 32].width()] );
            font[230 - 32].setPosition( font[88].x(), font[88].y() );
            updateNormalFontLetterShadow( font[230 - 32] );

            // letter 3 (z)
            font[231 - 32].resize( font[19].width(), font[19].height() - 4 );
            font[231 - 32].reset();
            fheroes2::Copy( font[19], 0, 0, font[231 - 32], 0, 0, font[19].width(), 3 );
            fheroes2::Copy( font[19], 0, 5, font[231 - 32], 0, 3, font[19].width(), 1 );
            fheroes2::Copy( font[19], 0, 8, font[231 - 32], 0, 4, font[19].width(), 4 );
            fheroes2::FillTransform( font[231 - 32], 0, 2, 3, 3, 1 );
            font[231 - 32].setPosition( font[19].x(), font[19].y() + 4 );
            updateNormalFontLetterShadow( font[231 - 32] );

            // letter B (v)
            font[226 - 32].resize( font[231 - 32].width() + 1, font[231 - 32].height() );
            font[226 - 32].reset();
            fheroes2::Copy( font[231 - 32], 0, 0, font[226 - 32], 1, 0, font[231 - 32].width(), font[231 - 32].height() );
            fheroes2::Copy( font[77], 1, 0, font[226 - 32], 1, 0, 3, 7 );
            fheroes2::Copy( font[226 - 32], 7, 1, font[226 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[226 - 32], 7, 1, font[226 - 32], 3, 6, 1, 1 );
            fheroes2::Copy( font[226 - 32], 3, 4, font[226 - 32], 3, 5, 1, 1 );
            font[226 - 32].setPosition( font[231 - 32].x(), font[231 - 32].y() );
            updateNormalFontLetterShadow( font[226 - 32] );

            font[232 - 32] = font[85];

            font[233 - 32].resize( font[232 - 32].width(), font[232 - 32].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[232 - 32], 0, 0, font[233 - 32], 0, 3, font[232 - 32].width(), font[232 - 32].height() );
            fheroes2::Copy( font[233 - 32], 8, 3, font[233 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[233 - 32], 7, 3, font[233 - 32], 5, 1, 1, 1 );
            font[233 - 32].setPosition( font[232 - 32].x(), font[232 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[233 - 32] );

            // Shorter k.
            font[234 - 32].resize( font[75].width() - 1, font[75].height() - 4 );
            font[234 - 32].reset();
            fheroes2::Copy( font[75], 0, 0, font[234 - 32], 0, 0, 4, 6 );
            fheroes2::Copy( font[75], 4, 4, font[234 - 32], 4, 0, 5, 6 );
            fheroes2::Copy( font[75], 0, 10, font[234 - 32], 0, 6, 4, 1 );
            fheroes2::Copy( font[75], 7, 10, font[234 - 32], 6, 6, 3, 1 );
            font[234 - 32].setPosition( font[75].x(), font[75].y() + 4 );
            updateNormalFontLetterShadow( font[234 - 32] );

            font[235 - 32] = font[78];
            fheroes2::Copy( font[235 - 32], 3, 0, font[235 - 32], 2, 1, 1, 1 );
            fheroes2::FillTransform( font[235 - 32], 0, 0, 2, 3, 1 );
            fheroes2::FillTransform( font[235 - 32], 2, 0, 1, 1, 1 );
            updateNormalFontLetterShadow( font[235 - 32] );

            font[236 - 32] = font[45 + offset];
            fheroes2::Copy( font[87], 9, 0, font[236 - 32], 3, 0, 4, 7 );
            fheroes2::Copy( font[87], 9, 0, font[236 - 32], 9, 0, 4, 7 );
            fheroes2::FillTransform( font[236 - 32], 0, 0, 3, 6, 1 );
            updateNormalFontLetterShadow( font[236 - 32] );

            font[237 - 32] = font[78];
            fheroes2::FillTransform( font[237 - 32], 4, 0, 3, 8, 1 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 4, 3, 1, 2 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 5, 3, 1, 2 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 6, 3, 1, 2 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 7, 3, 1, 1 );
            updateNormalFontLetterShadow( font[237 - 32] );

            font[238 - 32] = font[47 + offset];

            font[239 - 32] = font[78];

            font[240 - 32] = font[48 + offset];
            font[241 - 32] = font[35 + offset];
            font[242 - 32] = font[77];
            font[243 - 32] = font[57 + offset];

            font[244 - 32].resize( font[81].width(), font[81].height() );
            font[244 - 32].reset();
            fheroes2::Copy( font[80], 1, 0, font[244 - 32], 3, 0, 4, 10 );
            fheroes2::Copy( font[81], 0, 0, font[244 - 32], 0, 0, 5, 7 );
            fheroes2::Copy( font[80], 6, 0, font[244 - 32], 7, 0, 4, 7 );
            font[244 - 32].setPosition( font[81].x(), font[81].y() );
            updateNormalFontLetterShadow( font[244 - 32] );

            font[245 - 32] = font[56 + offset];

            font[246 - 32].resize( font[85].width() + 2, font[85].height() + 1 );
            font[246 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[246 - 32], 0, 0, font[85].width(), font[85].height() );
            fheroes2::Copy( font[246 - 32], 7, 4, font[246 - 32], 9, 5, 1, 1 );
            fheroes2::Copy( font[246 - 32], 7, 4, font[246 - 32], 10, 4, 1, 1 );
            fheroes2::Copy( font[246 - 32], 8, 1, font[246 - 32], 11, 4, 1, 4 );
            fheroes2::Copy( font[246 - 32], 8, 1, font[246 - 32], 10, 5, 1, 1 );
            fheroes2::Copy( font[246 - 32], 9, 5, font[246 - 32], 10, 6, 1, 1 );
            fheroes2::Copy( font[246 - 32], 9, 5, font[246 - 32], 10, 7, 1, 1 );
            font[246 - 32].setPosition( font[85].x(), font[85].y() );
            updateNormalFontLetterShadow( font[246 - 32] );

            font[247 - 32] = font[85];
            fheroes2::Copy( font[247 - 32], 2, 5, font[247 - 32], 2, 3, 6, 2 );
            fheroes2::Copy( font[247 - 32], 8, 0, font[247 - 32], 7, 4, 1, 1 );
            fheroes2::Copy( font[247 - 32], 8, 0, font[247 - 32], 7, 5, 1, 1 );
            fheroes2::Copy( font[247 - 32], 8, 0, font[247 - 32], 7, 6, 1, 1 );
            fheroes2::FillTransform( font[247 - 32], 1, 5, 6, 4, 1 );
            updateNormalFontLetterShadow( font[247 - 32] );

            font[248 - 32].resize( font[85].width() + 3, font[85].height() );
            font[248 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[248 - 32], 0, 0, 4, 7 );
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 5, 0, 4, 7 );
            fheroes2::Copy( font[85], 6, 0, font[248 - 32], 9, 0, 4, 7 );
            fheroes2::Copy( font[248 - 32], 8, 5, font[248 - 32], 4, 5, 4, 2 );
            font[248 - 32].setPosition( font[85].x(), font[85].y() );
            updateNormalFontLetterShadow( font[248 - 32] );

            font[249 - 32].resize( font[248 - 32].width() + 2, font[248 - 32].height() );
            font[249 - 32].reset();
            fheroes2::Copy( font[248 - 32], 0, 0, font[249 - 32], 0, 0, 12, 7 );
            fheroes2::Copy( font[246 - 32], 9, 4, font[249 - 32], 12, 4, 3, 4 );
            font[249 - 32].setPosition( font[248 - 32].x(), font[248 - 32].y() );
            updateNormalFontLetterShadow( font[249 - 32] );

            font[252 - 32] = font[226 - 32];
            fheroes2::FillTransform( font[252 - 32], 4, 0, 5, 3, 1 );

            font[250 - 32].resize( font[252 - 32].width() + 1, font[252 - 32].height() );
            font[250 - 32].reset();
            fheroes2::Copy( font[252 - 32], 0, 0, font[250 - 32], 1, 0, font[252 - 32].width(), font[252 - 32].height() );
            fheroes2::Copy( font[252 - 32], 1, 0, font[250 - 32], 1, 0, 1, 2 );
            font[250 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            updateNormalFontLetterShadow( font[250 - 32] );

            font[251 - 32].resize( font[252 - 32].width() + 3, font[252 - 32].height() );
            font[251 - 32].reset();
            fheroes2::Copy( font[252 - 32], 0, 0, font[251 - 32], 0, 0, font[252 - 32].width(), font[252 - 32].height() );
            fheroes2::Copy( font[252 - 32], 2, 0, font[251 - 32], 10, 0, 2, 7 );
            font[251 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            updateNormalFontLetterShadow( font[251 - 32] );

            font[253 - 32] = font[79];
            fheroes2::FillTransform( font[253 - 32], 0, 2, 3, 3, 1 );
            fheroes2::Copy( font[253 - 32], 8, 3, font[253 - 32], 7, 3, 1, 1 );
            fheroes2::Copy( font[253 - 32], 8, 3, font[253 - 32], 6, 3, 1, 1 );
            fheroes2::Copy( font[253 - 32], 8, 3, font[253 - 32], 5, 3, 1, 1 );
            updateNormalFontLetterShadow( font[253 - 32] );

            font[254 - 32].resize( font[79].width() + 1, font[79].height() );
            font[254 - 32].reset();
            fheroes2::Copy( font[251 - 32], 1, 0, font[254 - 32], 1, 0, 3, 7 );
            fheroes2::Copy( font[79], 2, 1, font[254 - 32], 6, 1, 1, 5 );
            fheroes2::Copy( font[79], 3, 0, font[254 - 32], 6, 0, 3, 2 );
            fheroes2::Copy( font[79], 7, 0, font[254 - 32], 9, 0, 1, 2 );
            fheroes2::Copy( font[79], 8, 2, font[254 - 32], 9, 2, 1, 3 );
            fheroes2::Copy( font[79], 7, 5, font[254 - 32], 9, 5, 1, 2 );
            fheroes2::Copy( font[79], 3, 6, font[254 - 32], 6, 6, 3, 1 );
            fheroes2::Copy( font[254 - 32], 1, 0, font[254 - 32], 4, 3, 2, 1 );
            font[254 - 32].setPosition( font[251 - 32].x(), font[251 - 32].y() );
            updateNormalFontLetterShadow( font[254 - 32] );

            font[255 - 32] = font[65];
            fheroes2::FillTransform( font[255 - 32], 0, 2, 6, 3, 1 );
            fheroes2::Copy( font[69], 2, 5, font[255 - 32], 1, 2, 6, 2 );
            fheroes2::Copy( font[255 - 32], 6, 4, font[255 - 32], 6, 3, 1, 1 );
            updateNormalFontLetterShadow( font[255 - 32] );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            size_t offset = 0;

            // ' (right single quotation mark)
            font[146 - 32] = font[44 - 32];
            font[146 - 32].setPosition( font[146 - 32].x(), font[146 - 32].y() - 4 );

            // E with 2 dots on top.
            font[168 - 32].resize( font[37].width(), font[37].height() + 2 );
            font[168 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[168 - 32], 0, 2, font[37].width(), font[37].height() );
            fheroes2::Copy( font[37], 3, 0, font[168 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[37], 3, 0, font[168 - 32], 5, 0, 1, 1 );
            font[168 - 32].setPosition( font[37].x(), font[37].y() );
            updateSmallFontLetterShadow( font[168 - 32] );

            font[161 - 32].resize( font[57 + offset].width(), font[57 + offset].height() + 2 );
            font[161 - 32].reset();
            fheroes2::Copy( font[57 + offset], 0, 0, font[161 - 32], 0, 2, font[57 + offset].width(), font[57 + offset].height() );
            fheroes2::Copy( font[57 + offset], 2, 0, font[161 - 32], 5, 0, 2, 1 );
            font[161 - 32].setPosition( font[57 + offset].x(), font[57 + offset].y() - 2 );
            updateSmallFontLetterShadow( font[161 - 32] );

            font[162 - 32].resize( font[89 + offset].width(), font[89 + offset].height() + 2 );
            font[162 - 32].reset();
            fheroes2::Copy( font[89 + offset], 0, 0, font[162 - 32], 0, 2, font[89 + offset].width(), font[89 + offset].height() );
            fheroes2::Copy( font[89 + offset], 1, 0, font[162 - 32], 4, 0, 2, 1 );
            font[162 - 32].setPosition( font[89 + offset].x(), font[89 + offset].y() - 2 );
            updateSmallFontLetterShadow( font[162 - 32] );

            // C with a horizontal line in the middle.
            font[170 - 32] = font[67 - 32];
            fheroes2::Copy( font[170 - 32], 3, 0, font[170 - 32], 2, 3, 3, 1 );
            updateSmallFontLetterShadow( font[170 - 32] );

            font[186 - 32] = font[99 - 32];
            fheroes2::Copy( font[186 - 32], 2, 0, font[186 - 32], 2, 2, 2, 1 );
            updateSmallFontLetterShadow( font[186 - 32] );

            // I and i with 2 dots.
            font[175 - 32] = font[73 - 32];

            font[191 - 32] = font[105 - 32];
            fheroes2::Copy( font[105 - 32], 1, 0, font[191 - 32], 0, 0, 2, 2 );
            fheroes2::Copy( font[105 - 32], 1, 0, font[191 - 32], 2, 0, 2, 2 );

            // J and j.
            font[163 - 32] = font[74 - 32];
            font[188 - 32] = font[106 - 32];

            // S and s.
            font[189 - 32] = font[83 - 32];
            font[190 - 32] = font[115 - 32];

            // I and i.
            font[178 - 32] = font[73 - 32];
            font[179 - 32] = font[105 - 32];

            // A.
            font[192 - 32] = font[33 + offset];

            font[193 - 32] = font[34 + offset];
            fheroes2::FillTransform( font[193 - 32], 5, 1, 2, 2, 1 );
            fheroes2::Copy( font[193 - 32], 5, 0, font[193 - 32], 6, 0, 1, 1 );
            updateSmallFontLetterShadow( font[193 - 32] );

            font[194 - 32] = font[34 + offset];

            font[195 - 32].resize( font[193 - 32].width() + 1, font[193 - 32].height() );
            font[195 - 32].reset();
            fheroes2::Copy( font[193 - 32], 0, 0, font[195 - 32], 0, 0, 4, 8 );
            fheroes2::Copy( font[193 - 32], 3, 0, font[195 - 32], 4, 0, 4, 1 );
            font[195 - 32].setPosition( font[193 - 32].x(), font[193 - 32].y() );
            updateSmallFontLetterShadow( font[195 - 32] );

            // The same letter as above but with a vertical line of top.
            font[165 - 32].resize( font[195 - 32].width() - 1, font[195 - 32].height() + 1 );
            font[165 - 32].reset();
            fheroes2::Copy( font[195 - 32], 0, 0, font[165 - 32], 0, 1, font[195 - 32].width() - 1, font[195 - 32].height() );
            fheroes2::Copy( font[195 - 32], 2, 0, font[165 - 32], 6, 0, 1, 1 );
            fheroes2::FillTransform( font[165 - 32], 6, 2, 1, 1, 1 );
            font[165 - 32].setPosition( font[195 - 32].x(), font[195 - 32].y() - 1 );
            updateSmallFontLetterShadow( font[165 - 32] );

            font[196 - 32] = font[36 + offset];
            font[197 - 32] = font[37 + offset];

            font[198 - 32].resize( font[56].width() + 1, font[56].height() );
            font[198 - 32].reset();
            fheroes2::Copy( font[56], 1, 0, font[198 - 32], 1, 0, 3, 7 );
            fheroes2::Copy( font[56], 7, 0, font[198 - 32], 7, 0, 2, 7 );
            fheroes2::Copy( font[56], 4, 2, font[198 - 32], 3, 2, 1, 3 );
            fheroes2::Copy( font[56], 6, 2, font[198 - 32], 7, 2, 1, 3 );
            fheroes2::Copy( font[37], 4, 5, font[198 - 32], 4, 2, 3, 3 );
            fheroes2::Copy( font[37], 3, 0, font[198 - 32], 5, 0, 1, 7 );
            fheroes2::Copy( font[56], 8, 0, font[198 - 32], 9, 0, 1, 7 );
            font[198 - 32].setPosition( font[56].x(), font[56].y() );
            updateSmallFontLetterShadow( font[198 - 32] );

            font[199 - 32].resize( font[19].width() + 2, font[19].height() );
            font[199 - 32].reset();
            fheroes2::Copy( font[19], 1, 0, font[199 - 32], 1, 0, 3, 2 );
            fheroes2::Copy( font[19], 2, 0, font[199 - 32], 4, 0, 3, 2 );
            fheroes2::Copy( font[19], 2, 2, font[199 - 32], 3, 2, 3, 3 );
            fheroes2::Copy( font[19], 2, 5, font[199 - 32], 4, 5, 3, 2 );
            fheroes2::Copy( font[19], 1, 5, font[199 - 32], 1, 5, 3, 2 );
            fheroes2::FillTransform( font[199 - 32], 2, 4, 3, 2, 1 );
            fheroes2::FillTransform( font[199 - 32], 5, 5, 1, 1, 1 );
            fheroes2::FillTransform( font[199 - 32], 4, 2, 1, 1, 1 );
            font[199 - 32].setPosition( font[19].x(), font[19].y() );
            updateSmallFontLetterShadow( font[199 - 32] );

            font[200 - 32] = font[40];
            fheroes2::FillTransform( font[200 - 32], 4, 2, 3, 4, 1 );
            fheroes2::Copy( font[40], 3, 0, font[200 - 32], 4, 4, 1, 1 );
            fheroes2::Copy( font[40], 3, 0, font[200 - 32], 5, 3, 1, 1 );
            fheroes2::Copy( font[40], 3, 0, font[200 - 32], 6, 2, 1, 1 );
            updateSmallFontLetterShadow( font[200 - 32] );

            font[201 - 32].resize( font[200 - 32].width(), font[200 - 32].height() + 2 );
            font[201 - 32].reset();
            fheroes2::Copy( font[200 - 32], 1, 0, font[201 - 32], 1, 2, 8, 7 );
            fheroes2::Copy( font[200 - 32], 2, 0, font[201 - 32], 5, 0, 2, 1 );
            font[201 - 32].setPosition( font[200 - 32].x(), font[200 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[201 - 32] );

            font[202 - 32] = font[43 + offset];

            font[203 - 32].resize( font[34].width(), font[34].height() );
            font[203 - 32].reset();
            fheroes2::Copy( font[34], 1, 0, font[203 - 32], 1, 0, 3, 7 );
            fheroes2::Copy( font[34], 3, 0, font[203 - 32], 6, 0, 1, 7 );
            fheroes2::Copy( font[34], 3, 0, font[203 - 32], 4, 0, 2, 1 );
            fheroes2::FillTransform( font[203 - 32], 1, 0, 2, 2, 1 );
            fheroes2::FillTransform( font[203 - 32], 3, 0, 1, 1, 1 );
            font[203 - 32].setPosition( font[34].x(), font[34].y() );
            updateSmallFontLetterShadow( font[203 - 32] );

            font[204 - 32] = font[45 + offset];
            font[205 - 32] = font[40 + offset];
            font[206 - 32] = font[47 + offset];

            font[207 - 32] = font[195 - 32];
            fheroes2::Copy( font[207 - 32], 3, 0, font[207 - 32], 6, 0, 1, 7 );
            updateSmallFontLetterShadow( font[207 - 32] );

            font[208 - 32] = font[48 + offset];
            font[209 - 32] = font[35 + offset];

            font[210 - 32].resize( font[207 - 32].width() + 2, font[207 - 32].height() );
            font[210 - 32].reset();
            fheroes2::Copy( font[207 - 32], 0, 0, font[210 - 32], 0, 0, font[207 - 32].width(), font[207 - 32].height() );
            fheroes2::Copy( font[210 - 32], 5, 0, font[210 - 32], 8, 0, 2, 8 );
            font[210 - 32].setPosition( font[207 - 32].x(), font[207 - 32].y() );

            font[211 - 32] = font[57 + offset];
            font[213 - 32] = font[56 + offset];

            font[214 - 32].resize( font[53].width(), font[53].height() + 1 );
            font[214 - 32].reset();
            fheroes2::Copy( font[53], 1, 0, font[214 - 32], 1, 0, 8, 7 );
            fheroes2::Copy( font[214 - 32], 3, 0, font[214 - 32], 9, 5, 1, 3 );
            font[214 - 32].setPosition( font[53].x(), font[53].y() );
            updateSmallFontLetterShadow( font[214 - 32] );

            font[215 - 32] = font[53];
            fheroes2::Copy( font[53], 3, 5, font[215 - 32], 3, 2, 4, 2 );
            fheroes2::FillTransform( font[215 - 32], 2, 4, 5, 4, 1 );
            updateSmallFontLetterShadow( font[215 - 32] );

            font[216 - 32].resize( font[53].width(), font[53].height() );
            font[216 - 32].reset();
            fheroes2::Copy( font[53], 1, 0, font[216 - 32], 1, 0, 4, 7 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 6, 1, 2, 6 );
            fheroes2::Copy( font[44], 3, 0, font[216 - 32], 9, 0, 1, 8 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 5, 5, 1, 1 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 8, 5, 1, 1 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 6, 0, 1, 1 );
            font[216 - 32].setPosition( font[53].x(), font[53].y() );
            updateSmallFontLetterShadow( font[216 - 32] );

            font[217 - 32].resize( font[216 - 32].width() + 2, font[216 - 32].height() + 1 );
            font[217 - 32].reset();
            fheroes2::Copy( font[216 - 32], 1, 0, font[217 - 32], 1, 0, 9, 7 );
            fheroes2::Copy( font[216 - 32], 3, 0, font[217 - 32], 10, 6, 1, 1 );
            fheroes2::Copy( font[216 - 32], 3, 0, font[217 - 32], 11, 5, 1, 3 );
            font[217 - 32].setPosition( font[216 - 32].x(), font[216 - 32].y() );
            updateSmallFontLetterShadow( font[217 - 32] );

            font[220 - 32].resize( font[34].width(), font[34].height() );
            font[220 - 32].reset();
            fheroes2::Copy( font[34], 2, 0, font[220 - 32], 1, 0, 2, 7 );
            fheroes2::Copy( font[34], 4, 3, font[220 - 32], 3, 3, 1, 4 );
            fheroes2::Copy( font[34], 4, 3, font[220 - 32], 4, 3, 3, 4 );
            fheroes2::FillTransform( font[220 - 32], 1, 0, 1, 1, 1 );
            font[220 - 32].setPosition( font[34].x(), font[34].y() );
            updateSmallFontLetterShadow( font[220 - 32] );

            font[219 - 32].resize( font[220 - 32].width() + 2, font[220 - 32].height() );
            font[219 - 32].reset();
            fheroes2::Copy( font[220 - 32], 1, 0, font[219 - 32], 1, 0, 6, 7 );
            fheroes2::Copy( font[220 - 32], 2, 0, font[219 - 32], 8, 0, 1, 7 );
            font[219 - 32].setPosition( font[220 - 32].x(), font[220 - 32].y() );
            updateSmallFontLetterShadow( font[219 - 32] );

            font[218 - 32].resize( font[220 - 32].width() + 2, font[220 - 32].height() );
            font[218 - 32].reset();
            fheroes2::Copy( font[220 - 32], 1, 0, font[218 - 32], 3, 0, 6, 7 );
            fheroes2::Copy( font[220 - 32], 2, 3, font[218 - 32], 1, 0, 3, 1 );
            fheroes2::Copy( font[220 - 32], 2, 3, font[218 - 32], 1, 1, 1, 1 );
            font[218 - 32].setPosition( font[220 - 32].x(), font[220 - 32].y() );
            updateSmallFontLetterShadow( font[218 - 32] );

            font[221 - 32].resize( font[47].width() - 1, font[47].height() );
            font[221 - 32].reset();
            fheroes2::Copy( font[47], 2, 0, font[221 - 32], 1, 0, 6, 7 );
            fheroes2::Copy( font[47], 3, 0, font[221 - 32], 4, 3, 2, 1 );
            font[221 - 32].setPosition( font[47].x(), font[47].y() );
            updateSmallFontLetterShadow( font[221 - 32] );

            font[222 - 32].resize( font[47].width() + 1, font[47].height() );
            font[222 - 32].reset();
            fheroes2::Copy( font[44], 2, 0, font[222 - 32], 1, 0, 2, 7 );
            fheroes2::Copy( font[47], 2, 0, font[222 - 32], 4, 0, 5, 2 );
            fheroes2::Copy( font[47], 2, 5, font[222 - 32], 4, 5, 5, 2 );
            fheroes2::Copy( font[222 - 32], 1, 0, font[222 - 32], 3, 3, 1, 1 );
            fheroes2::Copy( font[222 - 32], 2, 0, font[222 - 32], 4, 2, 1, 3 );
            fheroes2::Copy( font[222 - 32], 2, 0, font[222 - 32], 8, 2, 1, 3 );
            font[222 - 32].setPosition( font[47].x(), font[47].y() );
            updateSmallFontLetterShadow( font[222 - 32] );

            font[223 - 32].resize( font[48].width() - 1, font[48].height() );
            font[223 - 32].reset();
            fheroes2::Copy( font[48], 7, 1, font[223 - 32], 2, 1, 1, 2 );
            fheroes2::Copy( font[48], 2, 0, font[223 - 32], 3, 0, 3, 1 );
            fheroes2::Copy( font[48], 2, 0, font[223 - 32], 3, 3, 3, 1 );
            fheroes2::Copy( font[44], 3, 0, font[223 - 32], 6, 0, 1, 7 );
            fheroes2::Copy( font[33], 1, 6, font[223 - 32], 1, 6, 2, 1 );
            fheroes2::Copy( font[33], 1, 6, font[223 - 32], 3, 5, 1, 1 );
            fheroes2::Copy( font[33], 1, 6, font[223 - 32], 4, 4, 1, 1 );
            font[223 - 32].setPosition( font[48].x(), font[48].y() );
            updateSmallFontLetterShadow( font[223 - 32] );

            offset = 32;

            // e with 2 dots on top.
            font[184 - 32].resize( font[69].width(), font[69].height() + 2 );
            font[184 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[184 - 32], 0, 2, font[69].width(), font[69].height() );
            fheroes2::Copy( font[69], 2, 0, font[184 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[69], 2, 0, font[184 - 32], 4, 0, 1, 1 );
            font[184 - 32].setPosition( font[69].x(), font[69].y() - 2 );
            updateSmallFontLetterShadow( font[184 - 32] );

            font[224 - 32] = font[33 + offset];

            font[225 - 32].resize( font[34].width(), font[34].height() );
            font[225 - 32].reset();
            fheroes2::Copy( font[34], 4, 3, font[225 - 32], 4, 3, 3, 4 );
            fheroes2::Copy( font[65], 1, 2, font[225 - 32], 2, 4, 2, 3 );
            fheroes2::FillTransform( font[225 - 32], 3, 5, 1, 1, 1 );
            fheroes2::Copy( font[225 - 32], 2, 5, font[225 - 32], 2, 1, 2, 2 );
            fheroes2::Copy( font[37], 2, 0, font[225 - 32], 2, 0, 5, 1 );
            font[225 - 32].setPosition( font[34].x(), font[34].y() );
            updateSmallFontLetterShadow( font[225 - 32] );

            font[226 - 32].resize( font[82].width() - 1, font[82].height() );
            font[226 - 32].reset();
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 3, 0, 2, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 3, 2, 2, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 3, 4, 2, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 5, 3, 1, 1 );
            font[226 - 32].setPosition( font[82].x(), font[82].y() );
            updateSmallFontLetterShadow( font[226 - 32] );

            font[227 - 32] = font[82];
            fheroes2::Copy( font[227 - 32], 3, 1, font[227 - 32], 3, 0, 1, 1 );
            fheroes2::FillTransform( font[227 - 32], 3, 1, 1, 1, 1 );
            updateSmallFontLetterShadow( font[227 - 32] );

            // The same letter as above but with a vertical line of top.
            font[180 - 32].resize( font[227 - 32].width() - 1, font[227 - 32].height() + 1 );
            font[180 - 32].reset();
            fheroes2::Copy( font[227 - 32], 0, 0, font[180 - 32], 0, 1, font[227 - 32].width() - 1, font[227 - 32].height() );
            fheroes2::Copy( font[227 - 32], 5, 0, font[180 - 32], 5, 0, 1, 1 );
            fheroes2::FillTransform( font[180 - 32], 5, 2, 1, 1, 1 );
            font[180 - 32].setPosition( font[227 - 32].x(), font[227 - 32].y() - 1 );
            updateSmallFontLetterShadow( font[180 - 32] );

            font[228 - 32] = font[71];

            font[229 - 32] = font[37 + offset];

            font[230 - 32].resize( font[88].width() + 1, font[88].height() );
            font[230 - 32].reset();
            fheroes2::Copy( font[88], 0, 0, font[230 - 32], 0, 0, 4, 5 );
            fheroes2::Copy( font[88], 4, 0, font[230 - 32], 5, 0, 4, 5 );
            fheroes2::Copy( font[85], 2, 0, font[230 - 32], 4, 0, 1, 4 );
            fheroes2::Copy( font[85], 2, 0, font[230 - 32], 4, 4, 1, 1 );
            font[230 - 32].setPosition( font[88].x(), font[88].y() );
            updateSmallFontLetterShadow( font[230 - 32] );

            font[232 - 32] = font[85];

            font[233 - 32].resize( font[232 - 32].width(), font[232 - 32].height() + 2 );
            font[233 - 32].reset();
            fheroes2::Copy( font[232 - 32], 1, 0, font[233 - 32], 1, 2, 7, 5 );
            fheroes2::Copy( font[232 - 32], 1, 0, font[233 - 32], 3, 0, 2, 1 );
            font[233 - 32].setPosition( font[232 - 32].x(), font[232 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[233 - 32] );

            font[234 - 32].resize( font[75].width(), font[75].height() - 2 );
            font[234 - 32].reset();
            fheroes2::Copy( font[75], 1, 0, font[234 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[75], 3, 2, font[234 - 32], 3, 0, 3, 5 );
            font[234 - 32].setPosition( font[75].x(), font[75].y() + 2 );
            updateSmallFontLetterShadow( font[234 - 32] );

            font[235 - 32].resize( font[65].width() - 2, font[65].height() );
            font[235 - 32].reset();
            fheroes2::Copy( font[203 - 32], 2, 3, font[235 - 32], 1, 1, 2, 4 );
            fheroes2::Copy( font[203 - 32], 5, 0, font[235 - 32], 3, 0, 2, 5 );
            fheroes2::FillTransform( font[235 - 32], 1, 1, 1, 1, 1 );
            font[235 - 32].setPosition( font[65].x(), font[65].y() );
            updateSmallFontLetterShadow( font[235 - 32] );

            font[236 - 32].resize( font[235 - 32].width() + 3, font[235 - 32].height() );
            font[236 - 32].reset();
            fheroes2::Copy( font[235 - 32], 4, 0, font[236 - 32], 4, 0, 1, 5 );
            fheroes2::Copy( font[235 - 32], 1, 0, font[236 - 32], 1, 1, 3, 3 );
            fheroes2::Copy( font[236 - 32], 2, 0, font[236 - 32], 5, 0, 3, 5 );
            fheroes2::Copy( font[236 - 32], 4, 0, font[236 - 32], 1, 4, 1, 1 );
            font[236 - 32].setPosition( font[235 - 32].x(), font[235 - 32].y() );
            updateSmallFontLetterShadow( font[236 - 32] );

            font[237 - 32].resize( font[72].width() - 2, font[72].height() - 2 );
            font[237 - 32].reset();
            fheroes2::Copy( font[72], 1, 0, font[237 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[72], 2, 0, font[237 - 32], 5, 0, 1, 5 );
            fheroes2::Copy( font[72], 3, 2, font[237 - 32], 3, 2, 2, 1 );
            font[237 - 32].setPosition( font[72].x(), font[72].y() + 2 );
            updateSmallFontLetterShadow( font[237 - 32] );

            font[238 - 32] = font[47 + offset];
            font[239 - 32] = font[78];
            font[240 - 32] = font[48 + offset];
            font[241 - 32] = font[35 + offset];

            font[242 - 32].resize( font[77].width() - 4, font[77].height() );
            font[242 - 32].reset();
            fheroes2::Copy( font[77], 1, 0, font[242 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[77], 6, 1, font[242 - 32], 4, 1, 1, 4 );
            fheroes2::Copy( font[77], 10, 1, font[242 - 32], 6, 1, 2, 4 );
            fheroes2::Copy( font[77], 3, 0, font[242 - 32], 2, 0, 3, 1 );
            fheroes2::Copy( font[77], 3, 0, font[242 - 32], 5, 0, 1, 1 );
            font[242 - 32].setPosition( font[77].x(), font[77].y() );
            updateSmallFontLetterShadow( font[242 - 32] );

            font[243 - 32] = font[57 + offset];

            font[244 - 32].resize( font[81].width() + 3, font[81].height() + 1 );
            font[244 - 32].reset();
            fheroes2::Copy( font[81], 1, 0, font[244 - 32], 1, 0, 5, 7 );
            fheroes2::Copy( font[81], 2, 0, font[244 - 32], 6, 0, 4, 4 );
            fheroes2::Copy( font[81], 2, 4, font[244 - 32], 6, 4, 3, 1 );
            fheroes2::Copy( font[81], 2, 4, font[244 - 32], 5, 7, 1, 1 );
            font[244 - 32].setPosition( font[81].x(), font[81].y() );
            updateSmallFontLetterShadow( font[244 - 32] );

            // Bigger letter
            font[212 - 32] = font[244 - 32];
            fheroes2::Copy( font[212 - 32], 5, 1, font[212 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[212 - 32], 5, 1, font[212 - 32], 4, 7, 1, 1 );
            font[212 - 32].setPosition( font[48].x(), font[48].y() ); // copy from a big better
            updateSmallFontLetterShadow( font[212 - 32] );

            font[245 - 32] = font[56 + offset];

            font[246 - 32].resize( font[85].width() + 1, font[85].height() + 1 );
            font[246 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[246 - 32], 0, 0, font[85].width(), font[85].height() );
            fheroes2::Copy( font[246 - 32], 2, 0, font[246 - 32], 8, 3, 1, 3 );
            font[246 - 32].setPosition( font[85].x(), font[85].y() );
            updateSmallFontLetterShadow( font[246 - 32] );

            font[247 - 32] = font[85];
            fheroes2::Copy( font[85], 2, 4, font[247 - 32], 2, 2, 4, 1 );
            fheroes2::Copy( font[85], 1, 0, font[247 - 32], 6, 4, 1, 1 );
            fheroes2::FillTransform( font[247 - 32], 1, 3, 5, 3, 1 );
            updateSmallFontLetterShadow( font[247 - 32] );

            font[248 - 32].resize( font[85].width() + 2, font[85].height() );
            font[248 - 32].reset();
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 1, 0, 3, 5 );
            fheroes2::Copy( font[85], 6, 0, font[248 - 32], 5, 0, 2, 5 );
            fheroes2::Copy( font[85], 6, 0, font[248 - 32], 8, 0, 2, 5 );
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 4, 4, 1, 1 );
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 7, 4, 1, 1 );
            font[248 - 32].setPosition( font[85].x(), font[85].y() );
            updateSmallFontLetterShadow( font[248 - 32] );

            font[249 - 32].resize( font[248 - 32].width() + 1, font[248 - 32].height() );
            font[249 - 32].reset();
            fheroes2::Copy( font[248 - 32], 1, 0, font[249 - 32], 1, 0, 9, 5 );
            fheroes2::Copy( font[248 - 32], 2, 0, font[249 - 32], 10, 3, 1, 3 );
            font[249 - 32].setPosition( font[248 - 32].x(), font[248 - 32].y() );
            updateSmallFontLetterShadow( font[249 - 32] );

            font[252 - 32] = font[226 - 32];
            fheroes2::Copy( font[252 - 32], 1, 0, font[252 - 32], 5, 4, 1, 1 );
            fheroes2::FillTransform( font[252 - 32], 0, 0, 2, 2, 1 );
            fheroes2::FillTransform( font[252 - 32], 3, 0, 3, 2, 1 );

            font[250 - 32].resize( font[252 - 32].width() + 1, font[252 - 32].height() );
            font[250 - 32].reset();
            fheroes2::Copy( font[252 - 32], 1, 0, font[250 - 32], 2, 0, 5, 5 );
            fheroes2::Copy( font[252 - 32], 2, 2, font[250 - 32], 1, 0, 2, 1 );
            fheroes2::Copy( font[252 - 32], 2, 2, font[250 - 32], 1, 1, 1, 1 );
            font[250 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            updateSmallFontLetterShadow( font[250 - 32] );

            font[251 - 32].resize( font[252 - 32].width() + 2, font[252 - 32].height() );
            font[251 - 32].reset();
            fheroes2::Copy( font[252 - 32], 1, 0, font[251 - 32], 1, 0, 5, 5 );
            fheroes2::Copy( font[252 - 32], 2, 0, font[251 - 32], 7, 0, 1, 5 );
            font[251 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            updateSmallFontLetterShadow( font[251 - 32] );

            font[253 - 32].resize( font[79].width() - 1, font[79].height() );
            font[253 - 32].reset();
            fheroes2::Copy( font[79], 2, 0, font[253 - 32], 1, 0, 4, 5 );
            fheroes2::Copy( font[79], 2, 0, font[253 - 32], 2, 2, 2, 1 );
            font[253 - 32].setPosition( font[79].x(), font[79].y() );
            updateSmallFontLetterShadow( font[253 - 32] );

            font[231 - 32] = font[253 - 32];
            fheroes2::FillTransform( font[231 - 32], 0, 1, 3, 3, 1 );
            fheroes2::FillTransform( font[231 - 32], 4, 2, 1, 1, 1 );
            fheroes2::FillTransform( font[231 - 32], 1, 0, 1, 1, 1 );
            fheroes2::Copy( font[253 - 32], 1, 0, font[231 - 32], 1, 1, 1, 1 );
            updateSmallFontLetterShadow( font[231 - 32] );

            font[254 - 32].resize( font[79].width() + 2, font[79].height() );
            font[254 - 32].reset();
            fheroes2::Copy( font[82], 1, 0, font[254 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[79], 1, 0, font[254 - 32], 4, 0, 3, 5 );
            fheroes2::Copy( font[79], 5, 1, font[254 - 32], 7, 1, 1, 3 );
            fheroes2::Copy( font[79], 5, 1, font[254 - 32], 3, 2, 1, 1 );
            font[254 - 32].setPosition( font[79].x(), font[79].y() );
            updateSmallFontLetterShadow( font[254 - 32] );

            font[255 - 32].resize( font[65].width() - 1, font[65].height() );
            font[255 - 32].reset();
            fheroes2::Copy( font[65], 1, 2, font[255 - 32], 2, 0, 3, 3 );
            fheroes2::Copy( font[235 - 32], 4, 0, font[255 - 32], 5, 0, 1, 5 );
            fheroes2::Copy( font[33], 1, 5, font[255 - 32], 1, 3, 3, 2 );
            font[255 - 32].setPosition( font[65].x(), font[65].y() );
            updateSmallFontLetterShadow( font[255 - 32] );
        }
    }

    // CP1252 supports German, Italian, Spanish, Norwegian, Swedish and Danish (and French but OG has custom encoding)
    void generateCP1252Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );

            const fheroes2::Sprite firstSprite{ icnVsSprite[icnId][0] };
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, firstSprite );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // Inverted exclamation mark !.
            font[161 - 32].resize( font[33 - 32].width() + 1, font[33 - 32].height() + 3 );
            font[161 - 32].reset();
            fheroes2::Copy( font[33 - 32], 1, 0, font[161 - 32], 1, 3, font[33 - 32].width(), font[33 - 32].height() );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[161 - 32], false, true );
                fheroes2::Copy( temp, 0, 2, font[161 - 32], 1, 2, temp.width(), temp.height() );
            }
            font[161 - 32].setPosition( font[33 - 32].x(), font[33 - 32].y() + 2 );
            updateNormalFontLetterShadow( font[161 - 32] );

            // Left-pointing double angle quotation mark <<.
            font[171 - 32].resize( 8, 9 );
            font[171 - 32].reset();
            fheroes2::Copy( font[47 - 32], 5, 0, font[171 - 32], 1, 0, 3, 3 );
            fheroes2::Flip( font[47 - 32], 5, 0, font[171 - 32], 1, 4, 3, 3, false, true );
            fheroes2::Flip( font[47 - 32], 5, 2, font[171 - 32], 1, 3, 2, 1, true, false );
            fheroes2::Copy( font[47 - 32], 5, 2, font[171 - 32], 1, 1, 1, 1 );
            fheroes2::Copy( font[47 - 32], 5, 2, font[171 - 32], 1, 5, 1, 1 );
            // Second mark.
            fheroes2::Copy( font[171 - 32], 1, 0, font[171 - 32], 5, 0, 3, 7 );
            font[171 - 32].setPosition( font[33].x(), font[33].y() + 4 );
            updateNormalFontLetterShadow( font[171 - 32] );

            // Right-pointing double angle quotation mark >>.
            font[187 - 32].resize( 8, 9 );
            font[187 - 32].reset();
            fheroes2::Flip( font[171 - 32], 1, 0, font[187 - 32], 1, 0, 7, 7, true, false );
            // Remove old shadows
            fheroes2::FillTransform( font[187 - 32], 3, 6, 1, 1, 1 );
            fheroes2::FillTransform( font[187 - 32], 7, 6, 1, 1, 1 );
            fheroes2::FillTransform( font[187 - 32], 4, 5, 1, 1, 1 );
            font[187 - 32].setPosition( font[33].x(), font[33].y() + 4 );
            updateNormalFontLetterShadow( font[187 - 32] );

            // Inverted question mark ?.
            font[191 - 32].resize( font[63 - 32].width() + 1, font[63 - 32].height() );
            font[191 - 32].reset();
            fheroes2::Copy( font[63 - 32], 1, 0, font[191 - 32], 0, 0, font[63 - 32].width(), 11 );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[191 - 32], true, true );
                fheroes2::Copy( temp, 1, 2, font[191 - 32], 0, 0, temp.width(), temp.height() );
            }
            // Remove old shadows
            fheroes2::FillTransform( font[191 - 32], 6, 1, 1, 2, 1 );
            fheroes2::FillTransform( font[191 - 32], 5, 2, 1, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 2, 4, 1, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 3, 8, 5, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 4, 7, 3, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 8, 5, 1, 1, 1 );
            font[191 - 32].setPosition( font[63 - 32].x(), font[63 - 32].y() + 2 );
            updateNormalFontLetterShadow( font[191 - 32] );
            // Improve generated shadows
            fheroes2::FillTransform( font[191 - 32], 0, 8, 1, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 0, 12, 1, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 7, 12, 1, 1, 1 );

            // A with grave accent ` and generate the grave accent for further use.
            font[192 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[192 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[192 - 32], 0, 3, font[33].width(), font[33].height() );
            font[192 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            fheroes2::Copy( font[192 - 32], 3, 4, font[192 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[192 - 32], 3, 4, font[192 - 32], 8, 1, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[192 - 32], 3, 3, font[192 - 32], 10, 1, 1, 1 );
            updateNormalFontLetterShadow( font[192 - 32] );

            // A with acute accent. Generate the accent for further use.
            font[193 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[193 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[193 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[193 - 32], 3, 4, font[193 - 32], 10, 0, 1, 1 );
            fheroes2::Copy( font[193 - 32], 4, 4, font[193 - 32], 9, 0, 1, 1 );
            fheroes2::Copy( font[193 - 32], 3, 4, font[193 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[193 - 32], 4, 4, font[193 - 32], 8, 1, 1, 1 );
            fheroes2::Copy( font[193 - 32], 3, 3, font[193 - 32], 7, 1, 1, 1 );
            font[193 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateNormalFontLetterShadow( font[193 - 32] );

            // A with circumflex accent. Generation of accent for further use.
            font[194 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[194 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 7, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 0, font[194 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 0, font[194 - 32], 9, 0, 1, 1 );
            font[194 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[194 - 32] );

            // A with tilde accent ~. Generate accent for further use.
            font[195 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 4 );
            font[195 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[195 - 32], 0, 4, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[37 - 32], 7, 5, font[195 - 32], 4, 0, 5, 2 );
            fheroes2::Copy( font[37 - 32], 8, 8, font[195 - 32], 8, 2, 3, 1 );
            fheroes2::Copy( font[37 - 32], 10, 7, font[195 - 32], 10, 1, 2, 1 );
            font[195 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[195 - 32] );

            // A with 2 dots on top.
            font[196 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[196 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[196 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[196 - 32], 3, 1 + 3, font[196 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[196 - 32], 4, 1 + 3, font[196 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[196 - 32], 4, 0, font[196 - 32], 9, 0, 2, 1 );
            font[196 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateNormalFontLetterShadow( font[196 - 32] );

            // A with circle on top.
            font[197 - 32].resize( font[33].width(), font[33].height() + 2 );
            font[197 - 32].reset();
            fheroes2::Copy( font[33], 0, 3, font[197 - 32], 0, 5, font[33].width(), font[33].height() );
            fheroes2::FillTransform( font[197 - 32], 2, 5, 3, 3, 1 );
            fheroes2::Copy( font[80], 5, 6, font[197 - 32], 7, 4, 4, 1 );
            fheroes2::Copy( font[80], 5, 6, font[197 - 32], 7, 0, 4, 1 );
            fheroes2::Copy( font[80], 5, 6, font[197 - 32], 7, 2, 4, 1 );
            fheroes2::Copy( font[84], 1, 0, font[197 - 32], 7, 1, 1, 1 );
            fheroes2::Copy( font[84], 1, 0, font[197 - 32], 10, 1, 1, 1 );
            font[197 - 32].setPosition( font[33].x(), font[33].y() - 2 );
            updateNormalFontLetterShadow( font[197 - 32] );

            // A attached to E.
            font[198 - 32].resize( font[33].width() + 3, font[33].height() );
            font[198 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[198 - 32], 0, 0, font[33].width(), font[33].height() );
            fheroes2::Copy( font[37], 6, 0, font[198 - 32], 12, 0, 6, 4 );
            fheroes2::Copy( font[37], 5, 0, font[198 - 32], 10, 0, 2, 2 );
            fheroes2::Copy( font[37], 6, 4, font[198 - 32], 12, 4, 3, 2 );
            fheroes2::Copy( font[37], 6, 4, font[198 - 32], 15, 4, 1, 2 );
            fheroes2::Copy( font[37], 8, 9, font[198 - 32], 14, 9, 3, 2 );
            font[198 - 32].setPosition( font[33].x(), font[33].y() );
            updateNormalFontLetterShadow( font[198 - 32] );

            // C with cedilla.
            font[199 - 32].resize( font[35].width(), font[35].height() + 3 );
            font[199 - 32].reset();
            fheroes2::Copy( font[35], 0, 0, font[199 - 32], 0, 0, font[35].width(), font[35].height() );
            fheroes2::Copy( font[67], 2, 1, font[199 - 32], 7, 11, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[199 - 32], 8, 11, 1, 1 );
            fheroes2::Copy( font[67], 2, 6, font[199 - 32], 9, 11, 1, 1 );
            fheroes2::Copy( font[67], 2, 1, font[199 - 32], 8, 12, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[199 - 32], 9, 12, 1, 1 );
            fheroes2::Copy( font[67], 2, 1, font[199 - 32], 7, 13, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[199 - 32], 8, 13, 1, 1 );
            fheroes2::Copy( font[67], 3, 0, font[199 - 32], 9, 13, 1, 1 );
            font[199 - 32].setPosition( font[35].x(), font[35].y() );
            updateNormalFontLetterShadow( font[199 - 32] );

            // E with grave accent `.
            font[200 - 32].resize( font[37].width(), font[37].height() + 3 );
            font[200 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[200 - 32], 0, 3, font[37].width(), font[37].height() );
            font[200 - 32].setPosition( font[37].x(), font[37].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[200 - 32], 4, 0, 4, 2 );
            updateNormalFontLetterShadow( font[200 - 32] );

            // E with acute accent.
            font[201 - 32].resize( font[37].width(), font[37].height() + 3 );
            font[201 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[201 - 32], 0, 3, font[37].width(), font[37].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[201 - 32], 5, 0, 4, 2 );
            font[201 - 32].setPosition( font[37].x(), font[37].y() - 3 );
            updateNormalFontLetterShadow( font[201 - 32] );

            // E with circumflex accent.
            font[202 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[202 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[202 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[202 - 32], 5, 0, 3, 2 );
            font[202 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[202 - 32] );

            // E with diaeresis.
            font[203 - 32].resize( font[37].width(), font[37].height() + 3 );
            font[203 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[203 - 32], 0, 3, font[37].width(), font[37].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[203 - 32], 5, 0, 4, 2 );
            font[203 - 32].setPosition( font[37].x(), font[37].y() - 3 );
            updateNormalFontLetterShadow( font[203 - 32] );

            // I with grave accent `.
            font[204 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[204 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[204 - 32], 0, 3, font[41].width(), font[41].height() );
            font[204 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[204 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[204 - 32] );

            // I with accute accent.
            font[205 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[205 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[205 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[205 - 32], 3, 0, 4, 2 );
            font[205 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[205 - 32] );

            // I with circumflex.
            font[206 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[206 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[206 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[206 - 32], 3, 0, 3, 2 );
            font[206 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[206 - 32] );

            // I with diaeresis.
            font[207 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[207 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[207 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[207 - 32], 3, 0, 4, 2 );
            font[207 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[207 - 32] );

            // N with tilde ~.
            font[209 - 32].resize( font[46].width(), font[46].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[46], 0, 0, font[209 - 32], 0, 3, font[46].width(), font[46].height() );
            fheroes2::Copy( font[195 - 32], 4, 0, font[209 - 32], 4, 0, 8, 3 );
            font[209 - 32].setPosition( font[46].x(), font[46].y() - 3 );
            updateNormalFontLetterShadow( font[209 - 32] );

            // O with grave accent `.
            font[210 - 32].resize( font[47].width(), font[47].height() + 3 );
            font[210 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[210 - 32], 0, 3, font[47].width(), font[47].height() );
            font[210 - 32].setPosition( font[47].x(), font[47].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[210 - 32], 6, 0, 4, 2 );
            updateNormalFontLetterShadow( font[210 - 32] );

            // O with acute accent.
            font[211 - 32].resize( font[47].width(), font[47].height() + 3 );
            font[211 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[211 - 32], 0, 3, font[47].width(), font[47].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[211 - 32], 7, 0, 4, 2 );
            font[211 - 32].setPosition( font[47].x(), font[47].y() - 3 );
            updateNormalFontLetterShadow( font[211 - 32] );

            // O with circumflex accent.
            font[212 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[212 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[212 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[212 - 32], 7, 0, 3, 2 );
            font[212 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[212 - 32] );

            // O with tilde accent ~.
            font[213 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 4 );
            font[213 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[213 - 32], 0, 4, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[195 - 32], 4, 0, font[213 - 32], 4, 0, 8, 3 );
            font[213 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[213 - 32] );

            // O with 2 dots on top. TODO replace with previous accent generation in A.
            font[214 - 32].resize( font[47].width(), font[47].height() + 3 );
            font[214 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[214 - 32], 0, 3, font[47].width(), font[47].height() );
            fheroes2::Copy( font[214 - 32], 1, 2 + 3, font[214 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 2, 2 + 3, font[214 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 5, 0, font[214 - 32], 10, 0, 2, 1 );
            font[214 - 32].setPosition( font[47].x(), font[47].y() - 3 );
            updateNormalFontLetterShadow( font[214 - 32] );

            // O with / inside.
            font[216 - 32].resize( font[47].width() + 2, font[47].height() );
            font[216 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[216 - 32], 0, 0, font[47].width(), font[47].height() );
            fheroes2::Copy( font[56], 10, 0, font[216 - 32], 6, 3, 5, 5 );
            fheroes2::Copy( font[56], 13, 0, font[216 - 32], font[47].width() - 1, 0, 3, 2 );
            fheroes2::Copy( font[47], font[47].width() - 3, 1, font[216 - 32], font[47].width() - 3, 2, 2, 1 );
            fheroes2::Copy( font[33], 1, 7, font[216 - 32], 1, 7, 2, 4 );
            fheroes2::Copy( font[56], 10, 3, font[216 - 32], 3, font[47].height() - 4, 2, 2 );
            font[216 - 32].setPosition( font[47].x(), font[47].y() );
            updateNormalFontLetterShadow( font[216 - 32] );

            // U with grave accent `.
            font[217 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[217 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[217 - 32], 0, 3, font[53].width(), font[53].height() );
            font[217 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[217 - 32], 5, 0, 4, 2 );
            updateNormalFontLetterShadow( font[217 - 32] );

            // U with acute accent.
            font[218 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[218 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[218 - 32], 0, 3, font[53].width(), font[53].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[218 - 32], 5, 0, 4, 2 );
            font[218 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            updateNormalFontLetterShadow( font[218 - 32] );

            // U with circumflex.
            font[219 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[219 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[219 - 32], 0, 3, font[53].width(), font[53].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[219 - 32], 6, 0, 3, 2 );
            font[219 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            updateNormalFontLetterShadow( font[219 - 32] );

            // U with 2 dots on top.
            font[220 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[220 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[220 - 32], 0, 3, font[53].width(), font[53].height() );
            fheroes2::Copy( font[220 - 32], 1, 1 + 3, font[220 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 2, 1 + 3, font[220 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 4, 0, font[220 - 32], 9, 0, 2, 1 );
            font[220 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            updateNormalFontLetterShadow( font[220 - 32] );

            // Eszett.
            font[223 - 32].resize( font[34].width(), font[34].height() + 3 );
            font[223 - 32].reset();
            fheroes2::Copy( font[34], 1, 0, font[223 - 32], 0, 3, font[34].width() - 1, font[34].height() );
            fheroes2::FillTransform( font[223 - 32], 0, 0 + 3, 3, 10, 1 );
            fheroes2::Copy( font[223 - 32], 0, 0 + 3, font[223 - 32], 3, 0 + 3, 1, 1 );
            fheroes2::Copy( font[223 - 32], 3, 1 + 3, font[223 - 32], 4, 0 + 3, 1, 1 );
            fheroes2::Copy( font[223 - 32], 4, 3 + 3, font[223 - 32], 3, 7 + 3, 1, 3 );
            fheroes2::Copy( font[223 - 32], 4, 3 + 3, font[223 - 32], 2, 10 + 3, 1, 1 );
            fheroes2::Copy( font[223 - 32], 3, 3 + 3, font[223 - 32], 2, 7 + 3, 1, 3 );
            fheroes2::Copy( font[223 - 32], 3, 6 + 3, font[223 - 32], 1, 10 + 3, 1, 1 );
            fheroes2::Copy( font[223 - 32], 7, 4 + 3, font[223 - 32], 4, 8 + 3, 1, 1 );
            fheroes2::Copy( font[223 - 32], 7, 4 + 3, font[223 - 32], 3, 10 + 3, 1, 1 );
            fheroes2::Copy( font[223 - 32], 8, 5 + 3, font[223 - 32], 4, 9 + 3, 1, 1 );
            font[223 - 32].setPosition( font[34].x(), font[34].y() - 3 );
            updateNormalFontLetterShadow( font[223 - 32] );

            // a with grave accent `.
            font[224 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[224 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[224 - 32], 0, 3, font[65].width(), font[65].height() );
            font[224 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[224 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[224 - 32] );

            // a with acute accent.
            font[225 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[225 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[225 - 32], 3, 0, 4, 2 );
            font[225 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[225 - 32] );

            // a with circumflex accent.
            font[226 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[226 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[226 - 32], 3, 0, 3, 2 );
            font[226 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[226 - 32] );

            // a with tilde accent ~.
            font[227 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 4 );
            font[227 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[227 - 32], 0, 4, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[195 - 32], 4, 0, font[227 - 32], 1, 0, 8, 3 );
            font[227 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[227 - 32] );

            // a with 2 dots on top.
            font[228 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[228 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[228 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[228 - 32], 3, 0 + 3, font[228 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[228 - 32], 2, 1 + 3, font[228 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[228 - 32], 2, 1 + 3, font[228 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[228 - 32], 1, 0 + 3, font[228 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[228 - 32], 2, 0, font[228 - 32], 5, 0, 2, 2 );
            font[228 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[228 - 32] );

            // a with circle on top.
            font[229 - 32].resize( font[65].width(), font[65].height() + 5 );
            font[229 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[229 - 32], 0, 4, font[65].width(), font[65].height() );
            fheroes2::Copy( font[197 - 32], 7, 0, font[229 - 32], 2, 0, 1, 3 );
            fheroes2::Copy( font[197 - 32], 10, 0, font[229 - 32], 6, 0, 1, 3 );
            fheroes2::Copy( font[65], 2, 0, font[229 - 32], 3, 0, 3, 1 );
            fheroes2::Copy( font[65], 2, 0, font[229 - 32], 3, 2, 3, 1 );
            fheroes2::Copy( font[69], 3, 2, font[229 - 32], 3, 1, 3, 1 );
            font[229 - 32].setPosition( font[65].x(), font[65].y() - 4 );
            updateNormalFontLetterShadow( font[229 - 32] );

            // a attached to e.
            font[230 - 32].resize( font[65].width() + 4, font[65].height() );
            font[230 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[230 - 32], 0, 0, font[65].width(), font[65].height() );
            fheroes2::Copy( font[69], 3, 0, font[230 - 32], 8, 0, 6, 8 );
            font[230 - 32].setPosition( font[65].x(), font[65].y() );
            updateNormalFontLetterShadow( font[230 - 32] );

            // c with cedilla.
            font[231 - 32].resize( font[67].width(), font[67].height() + 3 );
            font[231 - 32].reset();
            fheroes2::Copy( font[67], 0, 0, font[231 - 32], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[199 - 32], 7, 11, font[231 - 32], 4, 7, 3, 3 );
            font[231 - 32].setPosition( font[67].x(), font[67].y() );
            updateNormalFontLetterShadow( font[231 - 32] );

            // e with grave accent `.
            font[232 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[232 - 32], 0, 3, font[69].width(), font[69].height() );
            font[232 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[232 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[232 - 32] );

            // e with acute accent.
            font[233 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[233 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[233 - 32], 3, 0, 4, 2 );
            font[233 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[233 - 32] );

            // e with circumflex accent.
            font[234 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[234 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[234 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[234 - 32], 4, 0, 3, 2 );
            font[234 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[234 - 32] );

            // e with diaeresis.
            font[235 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[235 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[235 - 32], 0, 3, font[69].width(), font[69].height() );
            font[235 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[235 - 32] );

            // i with grave accent `.
            font[236 - 32] = font[73];
            fheroes2::FillTransform( font[236 - 32], 0, 0, font[236 - 32].width(), 3, 1 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[236 - 32], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[236 - 32] );

            // i with acute accent.
            font[237 - 32] = font[73];
            fheroes2::FillTransform( font[237 - 32], 0, 0, font[237 - 32].width(), 3, 1 );
            fheroes2::Copy( font[193 - 32], 7, 0, font[237 - 32], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[237 - 32] );

            // i with circumflex.
            font[238 - 32] = font[73];
            fheroes2::FillTransform( font[238 - 32], 0, 0, font[238 - 32].width(), 3, 1 );
            fheroes2::Copy( font[194 - 32], 7, 0, font[238 - 32], 1, 0, 3, 2 );
            updateNormalFontLetterShadow( font[238 - 32] );

            // i with diaeresis.
            font[239 - 32] = font[73];
            fheroes2::FillTransform( font[239 - 32], 0, 0, font[239 - 32].width(), 3, 1 );
            fheroes2::Copy( font[193 - 32], 7, 0, font[239 - 32], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[239 - 32] );

            // n with tilde ~.
            font[241 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 4 );
            font[241 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[241 - 32], 0, 4, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[195 - 32], 4, 0, font[241 - 32], 1, 0, 8, 3 );
            font[241 - 32].setPosition( font[110 - 32].x(), font[110 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[241 - 32] );

            // o with grave accent `.
            font[242 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[242 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[242 - 32], 0, 3, font[79].width(), font[79].height() );
            font[242 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[242 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[242 - 32] );

            // o with acute accent.
            font[243 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[243 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[243 - 32], 3, 0, 4, 2 );
            font[243 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[243 - 32] );

            // o with circumflex accent.
            font[244 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[244 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[244 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[244 - 32], 4, 0, 3, 2 );
            font[244 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[244 - 32] );

            // o with tilde accent ~.
            font[245 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 4 );
            font[245 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[245 - 32], 0, 4, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[195 - 32], 4, 0, font[245 - 32], 1, 0, 8, 3 );
            font[245 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 4 );
            updateNormalFontLetterShadow( font[245 - 32] );

            // o with 2 dots on top.
            font[246 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[246 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[246 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[246 - 32], 4, 0 + 3, font[246 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 6, 1 + 3, font[246 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[246 - 32], 6, 1 + 3, font[246 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 4, 1 + 3, font[246 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[246 - 32], 2, 0, font[246 - 32], 6, 0, 2, 2 );
            font[246 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[246 - 32] );

            // o with / inside.
            font[248 - 32].resize( font[79].width(), font[79].height() );
            font[248 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[248 - 32], 0, 0, font[79].width(), font[79].height() );
            fheroes2::Copy( font[88], 5, 0, font[248 - 32], 3, 2, 4, 4 );
            fheroes2::Copy( font[79], 6, 5, font[248 - 32], 6, 5, 1, 1 );
            fheroes2::Copy( font[88], 1, 6, font[248 - 32], 1, 6, 2, 1 );
            fheroes2::Copy( font[88], 7, 0, font[248 - 32], 7, 0, 2, 1 );
            font[248 - 32].setPosition( font[79].x(), font[79].y() );
            updateNormalFontLetterShadow( font[248 - 32] );

            // u with grave accent `.
            font[249 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[249 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[249 - 32], 0, 3, font[85].width(), font[85].height() );
            font[249 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[249 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[249 - 32] );

            // u with acute accent.
            font[250 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[250 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[250 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[250 - 32], 3, 0, 4, 2 );
            font[250 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[250 - 32] );

            // u with circumflex.
            font[251 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[251 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[251 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[251 - 32], 4, 0, 3, 2 );
            font[251 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[251 - 32] );

            // u with 2 dots on top.
            font[252 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[252 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[252 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[252 - 32], 2, 0 + 3, font[252 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 1 + 3, font[252 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 1 + 3, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 3, 6 + 3, font[252 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 0, font[252 - 32], 6, 0, 2, 2 );
            font[252 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[252 - 32] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // Inverted exclamation mark !.
            font[161 - 32].resize( font[33 - 32].width(), font[33 - 32].height() );
            font[161 - 32].reset();
            fheroes2::Copy( font[33 - 32], 1, 0, font[161 - 32], 1, 0, font[33 - 32].width(), 7 );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[161 - 32], false, true );
                fheroes2::Copy( temp, 0, 1, font[161 - 32], 0, 0, temp.width(), temp.height() );
            }
            font[161 - 32].setPosition( font[33 - 32].x(), font[33 - 32].y() + 2 );
            updateSmallFontLetterShadow( font[161 - 32] );

            // Inverted question mark ?.
            font[191 - 32].resize( font[63 - 32].width(), font[63 - 32].height() );
            font[191 - 32].reset();
            fheroes2::Copy( font[63 - 32], 1, 0, font[191 - 32], 0, 0, font[63 - 32].width(), 7 );
            {
                const fheroes2::Image temp = fheroes2::Flip( font[191 - 32], true, true );
                fheroes2::Copy( temp, 0, 1, font[191 - 32], 0, 0, temp.width(), temp.height() );
            }
            // Remove old shadows
            fheroes2::FillTransform( font[191 - 32], 4, 1, 1, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 3, 5, 2, 1, 1 );
            fheroes2::FillTransform( font[191 - 32], 2, 4, 1, 1, 1 );
            font[191 - 32].setPosition( font[63 - 32].x(), font[63 - 32].y() + 2 );
            updateSmallFontLetterShadow( font[191 - 32] );

            // A with grave accent `. Generate grave accent for further use.
            font[192 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[192 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[192 - 32], 0, 4, font[33].width(), font[33].height() );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 6, 2, 1, 1 );
            font[192 - 32].setPosition( font[33].x(), font[33].y() - 4 );
            updateSmallFontLetterShadow( font[192 - 32] );

            // A with acute accent. Generate acute accent for further use
            font[193 - 32].resize( font[33].width(), font[33].height() + 4 );
            font[193 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[193 - 32], 0, 4, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 4, 0, font[193 - 32], 4, 2, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[193 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[193 - 32], 6, 0, 1, 1 );
            font[193 - 32].setPosition( font[33].x(), font[33].y() - 4 );
            updateSmallFontLetterShadow( font[193 - 32] );

            // A with circumflex accent. Generate accent for further use.
            font[194 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[194 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 6, 1, 1, 1 );
            font[194 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateSmallFontLetterShadow( font[194 - 32] );

            // A with tilde accent ~. Generate for further use.
            font[195 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[195 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[195 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[37 - 32], 5, 4, font[195 - 32], 3, 0, 3, 2 );
            fheroes2::Copy( font[37 - 32], 5, 4, font[195 - 32], 6, 1, 2, 2 );
            font[195 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[195 - 32] );

            // A with 2 dots on top.
            font[196 - 32].resize( font[33].width(), font[33].height() + 2 );
            font[196 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[196 - 32], 0, 2, font[33].width(), font[33].height() );
            fheroes2::Copy( font[196 - 32], 3, 0 + 2, font[196 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[196 - 32], 3, 0 + 2, font[196 - 32], 6, 0, 1, 1 );
            font[196 - 32].setPosition( font[33].x(), font[33].y() - 2 );
            updateSmallFontLetterShadow( font[196 - 32] );

            // A with circle on top.
            font[197 - 32].resize( font[33].width(), font[33].height() + 4 );
            font[197 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[197 - 32], 0, 2, font[33].width(), font[33].height() );
            fheroes2::FillTransform( font[197 - 32], 3, 2, 3, 1, 1 );
            fheroes2::FillTransform( font[197 - 32], 1, 2, 3, 3, 1 );
            // Generate circle for further use.
            fheroes2::Copy( font[69], 2, 0, font[197 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[69], 2, 0, font[197 - 32], 6, 1, 1, 1 );
            fheroes2::Copy( font[69], 2, 0, font[197 - 32], 5, 2, 1, 1 );
            fheroes2::Copy( font[69], 2, 0, font[197 - 32], 4, 1, 1, 1 );
            font[197 - 32].setPosition( font[33].x(), font[33].y() - 2 );
            updateSmallFontLetterShadow( font[197 - 32] );

            // A attached to E.
            font[198 - 32].resize( font[33].width() + 3, font[33].height() );
            font[198 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[198 - 32], 0, 0, font[33].width(), font[33].height() );
            fheroes2::Copy( font[37], 3, 0, font[198 - 32], 6, 0, 5, 4 );
            fheroes2::Copy( font[37], 1, 0, font[198 - 32], 9, 5, 2, 2 );
            font[198 - 32].setPosition( font[33].x(), font[33].y() );
            updateSmallFontLetterShadow( font[198 - 32] );

            // C with cedilla.
            font[199 - 32].resize( font[35].width(), font[35].height() + 3 );
            font[199 - 32].reset();
            fheroes2::Copy( font[35], 0, 0, font[199 - 32], 0, 0, font[35].width(), font[35].height() );
            fheroes2::Copy( font[35], 1, 1, font[199 - 32], 3, 7, 2, 2 );
            font[199 - 32].setPosition( font[35].x(), font[35].y() );
            updateSmallFontLetterShadow( font[199 - 32] );

            // E with grave accent `.
            font[200 - 32].resize( font[37].width(), font[37].height() + 4 );
            font[200 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[200 - 32], 0, 4, font[37].width(), font[37].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[200 - 32], 4, 0, 3, 3 );
            font[200 - 32].setPosition( font[37].x(), font[37].y() - 4 );
            updateSmallFontLetterShadow( font[200 - 32] );

            // E with acute accent.
            font[201 - 32].resize( font[37].width(), font[37].height() + 4 );
            font[201 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[201 - 32], 0, 4, font[37].width(), font[37].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[201 - 32], 3, 0, 3, 3 );
            font[201 - 32].setPosition( font[37].x(), font[37].y() - 4 );
            updateSmallFontLetterShadow( font[201 - 32] );

            // E with circumflex accent.
            font[202 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[202 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[202 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[202 - 32], 3, 0, 3, 2 );
            font[202 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[202 - 32] );

            // I with grave accent `.
            font[204 - 32].resize( font[41].width(), font[41].height() + 4 );
            font[204 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[204 - 32], 0, 4, font[41].width(), font[41].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[204 - 32], 1, 0, 3, 3 );
            font[204 - 32].setPosition( font[41].x(), font[41].y() - 4 );
            updateSmallFontLetterShadow( font[204 - 32] );

            // I with acute accent.
            font[205 - 32].resize( font[41].width(), font[41].height() + 4 );
            font[205 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[205 - 32], 0, 4, font[41].width(), font[41].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[205 - 32], 1, 0, 3, 3 );
            font[205 - 32].setPosition( font[41].x(), font[41].y() - 4 );
            updateSmallFontLetterShadow( font[205 - 32] );

            // I with circumflex
            font[206 - 32].resize( font[73 - 32].width() + 1, font[73 - 32].height() + 3 );
            font[206 - 32].reset();
            fheroes2::Copy( font[73 - 32], 0, 0, font[206 - 32], 0, 3, font[73 - 32].width(), font[73 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[206 - 32], 2, 0, 3, 2 );
            font[206 - 32].setPosition( font[73 - 32].x(), font[73 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[206 - 32] );

            // N with tilde ~.
            font[209 - 32].resize( font[46].width(), font[46].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[46], 0, 0, font[209 - 32], 0, 4, font[46].width(), font[46].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[209 - 32], 4, 0, 5, 3 );
            font[209 - 32].setPosition( font[46].x(), font[46].y() - 4 );
            updateSmallFontLetterShadow( font[209 - 32] );

            // O with grave accent `.
            font[210 - 32].resize( font[47].width(), font[47].height() + 4 );
            font[210 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[210 - 32], 0, 4, font[47].width(), font[47].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[210 - 32], 4, 0, 3, 3 );
            font[210 - 32].setPosition( font[47].x(), font[47].y() - 4 );
            updateSmallFontLetterShadow( font[210 - 32] );

            // O with acute accent.
            font[211 - 32].resize( font[47].width(), font[47].height() + 4 );
            font[211 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[211 - 32], 0, 4, font[47].width(), font[47].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[211 - 32], 3, 0, 3, 3 );
            font[211 - 32].setPosition( font[47].x(), font[47].y() - 4 );
            updateSmallFontLetterShadow( font[211 - 32] );

            // O with circumflex accent.
            font[212 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[212 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[212 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[212 - 32], 3, 0, 3, 2 );
            font[212 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[212 - 32] );

            // O with tilde accent ~.
            font[213 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[213 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[213 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[213 - 32], 2, 0, 5, 3 );
            font[213 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[213 - 32] );

            // O with 2 dots on top.
            font[214 - 32].resize( font[47].width(), font[47].height() + 2 );
            font[214 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[214 - 32], 0, 2, font[47].width(), font[47].height() );
            fheroes2::Copy( font[214 - 32], 3, 0 + 2, font[214 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 3, 0 + 2, font[214 - 32], 5, 0, 1, 1 );
            font[214 - 32].setPosition( font[47].x(), font[47].y() - 2 );
            updateSmallFontLetterShadow( font[214 - 32] );

            // O with / inside.
            font[216 - 32].resize( font[47].width(), font[47].height() );
            font[216 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[216 - 32], 0, 0, font[47].width(), font[47].height() );
            fheroes2::Copy( font[56], 6, 0, font[216 - 32], 3, 2, 3, 3 );
            fheroes2::Copy( font[56], 1, 0, font[216 - 32], font[47].width() - 1, 0, 1, 1 );
            fheroes2::Copy( font[56], 1, 0, font[216 - 32], 1, font[47].height() - 2, 1, 1 );
            font[216 - 32].setPosition( font[47].x(), font[47].y() );
            updateSmallFontLetterShadow( font[216 - 32] );

            // U with grave accent `.
            font[217 - 32].resize( font[53].width(), font[53].height() + 4 );
            font[217 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[217 - 32], 0, 4, font[53].width(), font[53].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[217 - 32], 4, 0, 3, 3 );
            font[217 - 32].setPosition( font[53].x(), font[53].y() - 4 );
            updateSmallFontLetterShadow( font[217 - 32] );

            // U with acute accent.
            font[218 - 32].resize( font[53].width(), font[53].height() + 4 );
            font[218 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[218 - 32], 0, 4, font[53].width(), font[53].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[218 - 32], 4, 0, 3, 3 );
            font[218 - 32].setPosition( font[53].x(), font[53].y() - 4 );
            updateSmallFontLetterShadow( font[218 - 32] );

            // U with circumflex.
            font[219 - 32].resize( font[85 - 32].width(), font[85 - 32].height() + 3 );
            font[219 - 32].reset();
            fheroes2::Copy( font[85 - 32], 0, 0, font[219 - 32], 0, 3, font[85 - 32].width(), font[85 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[219 - 32], 4, 0, 3, 2 );
            font[219 - 32].setPosition( font[85 - 32].x(), font[85 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[219 - 32] );

            // U with 2 dots on top.
            font[220 - 32].resize( font[53].width(), font[53].height() + 2 );
            font[220 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[220 - 32], 0, 2, font[53].width(), font[53].height() );
            fheroes2::Copy( font[220 - 32], 3, 0 + 2, font[220 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 3, 0 + 2, font[220 - 32], 6, 0, 1, 1 );
            font[220 - 32].setPosition( font[53].x(), font[53].y() - 2 );
            updateSmallFontLetterShadow( font[220 - 32] );

            // Eszett.
            font[223 - 32].resize( font[34].width(), font[34].height() + 2 );
            font[223 - 32].reset();
            fheroes2::Copy( font[34], 0, 0, font[223 - 32], 0, 2, font[34].width(), font[34].height() );
            fheroes2::FillTransform( font[223 - 32], 0, 0 + 2, 4, 9, 1 );
            fheroes2::Copy( font[223 - 32], 6, 1 + 2, font[223 - 32], 2, 1 + 2, 1, 5 );
            fheroes2::Copy( font[223 - 32], 4, 3 + 2, font[223 - 32], 2, 3 + 2, 2, 1 );
            fheroes2::Copy( font[223 - 32], 5, 0 + 2, font[223 - 32], 1, 6 + 2, 1, 1 );
            fheroes2::Copy( font[223 - 32], 5, 0 + 2, font[223 - 32], 3, 0 + 2, 1, 1 );
            fheroes2::Copy( font[223 - 32], 5, 0 + 2, font[223 - 32], 3, 6 + 2, 1, 1 );
            font[223 - 32].setPosition( font[34].x(), font[34].y() - 2 );
            updateSmallFontLetterShadow( font[223 - 32] );

            // a with grave accent `.
            font[224 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[224 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[224 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[224 - 32], 3, 0, 3, 2 );
            font[224 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[224 - 32] );

            // a with acute accent.
            font[225 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[225 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[225 - 32], 3, 0, 2, 2 );
            font[225 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[225 - 32] );

            // a with circumflex accent.
            font[226 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[226 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[226 - 32], 2, 0, 3, 2 );
            font[226 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[226 - 32] );

            // a with tilde accent ~.
            font[227 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 4 );
            font[227 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[227 - 32], 0, 4, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[227 - 32], 1, 0, 5, 3 );
            font[227 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 4 );
            updateSmallFontLetterShadow( font[227 - 32] );

            // a with 2 dots on top.
            font[228 - 32].resize( font[65].width(), font[65].height() + 2 );
            font[228 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[228 - 32], 0, 2, font[65].width(), font[65].height() );
            fheroes2::Copy( font[228 - 32], 3, 0 + 2, font[228 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[228 - 32], 3, 0 + 2, font[228 - 32], 5, 0, 1, 1 );
            font[228 - 32].setPosition( font[65].x(), font[65].y() - 2 );
            updateSmallFontLetterShadow( font[228 - 32] );

            // a with circle on top.
            font[229 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[229 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[229 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[197 - 32], 4, 0, font[229 - 32], 2, 0, 3, 3 );
            font[229 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[229 - 32] );

            // a attached to e.
            font[230 - 32].resize( font[65].width() + 3, font[65].height() );
            font[230 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[230 - 32], 0, 0, font[65].width(), font[65].height() );
            fheroes2::Copy( font[69], 2, 0, font[230 - 32], 6, 0, font[69].width() - 2, font[69].height() );
            font[230 - 32].setPosition( font[65].x(), font[65].y() );
            updateSmallFontLetterShadow( font[230 - 32] );

            // c with cedilla.
            font[231 - 32].resize( font[67].width(), font[67].height() + 3 );
            font[231 - 32].reset();
            fheroes2::Copy( font[67], 0, 0, font[231 - 32], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[199 - 32], 1, 1, font[231 - 32], 2, 5, 2, 2 );
            font[231 - 32].setPosition( font[67].x(), font[67].y() );
            updateSmallFontLetterShadow( font[231 - 32] );

            // e with grave accent `.
            font[232 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[232 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[232 - 32], 3, 0, 3, 2 );
            font[232 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[232 - 32] );

            // e with acute accent.
            font[233 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[233 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[233 - 32], 3, 0, 2, 2 );
            font[233 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[233 - 32] );

            // e with circumflex accent.
            font[234 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[234 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[234 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[234 - 32], 2, 0, 3, 2 );
            font[234 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[234 - 32] );

            // i with grave accent `.
            font[236 - 32].resize( font[73].width(), font[73].height() + 1 );
            font[236 - 32].reset();
            fheroes2::Copy( font[73], 0, 2, font[236 - 32], 0, 3, font[73].width(), 6 );
            fheroes2::Copy( font[192 - 32], 4, 0, font[236 - 32], 2, 0, 2, 2 );
            font[236 - 32].setPosition( font[73].x(), font[73].y() - 1 );
            updateSmallFontLetterShadow( font[236 - 32] );

            // i with acute accent.
            font[237 - 32].resize( font[73].width(), font[73].height() + 1 );
            font[237 - 32].reset();
            fheroes2::Copy( font[73], 0, 2, font[237 - 32], 0, 3, font[73].width(), 6 );
            fheroes2::Copy( font[193 - 32], 5, 0, font[237 - 32], 1, 0, 2, 2 );
            font[237 - 32].setPosition( font[73].x(), font[73].y() - 1 );
            updateSmallFontLetterShadow( font[237 - 32] );

            // i with circumflex.
            font[238 - 32].resize( font[105 - 32].width(), font[105 - 32].height() + 1 );
            font[238 - 32].reset();
            fheroes2::Copy( font[105 - 32], 0, 2, font[238 - 32], 0, 3, font[105 - 32].width(), font[105 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[238 - 32], 1, 0, 3, 2 );
            font[238 - 32].setPosition( font[105 - 32].x(), font[105 - 32].y() - 1 );
            updateSmallFontLetterShadow( font[238 - 32] );

            // n with tilde ~.
            font[241 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 4 );
            font[241 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[241 - 32], 0, 4, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[241 - 32], 2, 0, 5, 3 );
            font[241 - 32].setPosition( font[110 - 32].x(), font[110 - 32].y() - 4 );
            updateSmallFontLetterShadow( font[241 - 32] );

            // o with grave accent `.
            font[242 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[242 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[242 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[242 - 32], 3, 0, 3, 2 );
            font[242 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[242 - 32] );

            // o with acute accent.
            font[243 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[243 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[243 - 32], 3, 0, 2, 2 );
            font[243 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[243 - 32] );

            // o with circumflex accent.
            font[244 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[244 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[244 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[244 - 32], 2, 0, 3, 2 );
            font[244 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[244 - 32] );

            // o with tilde accent ~.
            font[245 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 4 );
            font[245 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[245 - 32], 0, 4, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[245 - 32], 1, 0, 5, 3 );
            font[245 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 4 );
            updateSmallFontLetterShadow( font[245 - 32] );

            // o with 2 dots on top.
            font[246 - 32].resize( font[79].width(), font[79].height() + 2 );
            font[246 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[246 - 32], 0, 2, font[79].width(), font[79].height() );
            fheroes2::Copy( font[246 - 32], 3, 0 + 2, font[246 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 3, 0 + 2, font[246 - 32], 4, 0, 1, 1 );
            font[246 - 32].setPosition( font[79].x(), font[79].y() - 2 );
            updateSmallFontLetterShadow( font[246 - 32] );

            // o with / inside.
            font[248 - 32].resize( font[79].width(), font[79].height() );
            font[248 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[248 - 32], 0, 0, font[79].width(), font[79].height() );
            fheroes2::Copy( font[56], 6, 0, font[248 - 32], 2, 1, 3, 3 );
            fheroes2::Copy( font[56], 1, 0, font[248 - 32], font[79].width() - 1, 0, 1, 1 );
            fheroes2::Copy( font[56], 1, 0, font[248 - 32], 1, font[79].height() - 2, 1, 1 );
            font[248 - 32].setPosition( font[79].x(), font[79].y() );
            updateSmallFontLetterShadow( font[248 - 32] );

            // u with grave accent `.
            font[249 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[249 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[249 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[249 - 32], 3, 0, 3, 2 );
            font[249 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[249 - 32] );

            // u with acute accent.
            font[250 - 32] = font[85];
            font[250 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[250 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[250 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[250 - 32], 3, 0, 2, 2 );
            font[250 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[250 - 32] );

            // u with circumflex accent.
            font[251 - 32].resize( font[117 - 32].width(), font[117 - 32].height() + 3 );
            font[251 - 32].reset();
            fheroes2::Copy( font[117 - 32], 0, 0, font[251 - 32], 0, 3, font[117 - 32].width(), font[117 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[251 - 32], 3, 0, 3, 2 );
            font[251 - 32].setPosition( font[117 - 32].x(), font[117 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[251 - 32] );

            // u with 2 dots on top.
            font[252 - 32].resize( font[85].width(), font[85].height() + 2 );
            font[252 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[252 - 32], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 6, 0, 1, 1 );
            font[252 - 32].setPosition( font[85].x(), font[85].y() - 2 );
            updateSmallFontLetterShadow( font[252 - 32] );
        }
    }

    void generateCP1254Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );

            const fheroes2::Sprite firstSprite{ icnVsSprite[icnId][0] };
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, firstSprite );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // C with cedilla.
            font[199 - 32].resize( font[35].width(), font[35].height() + 3 );
            font[199 - 32].reset();
            fheroes2::Copy( font[35], 0, 0, font[199 - 32], 0, 0, font[35].width(), font[35].height() );
            fheroes2::Copy( font[67], 2, 1, font[199 - 32], 7, 11, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[199 - 32], 8, 11, 1, 1 );
            fheroes2::Copy( font[67], 2, 6, font[199 - 32], 9, 11, 1, 1 );
            fheroes2::Copy( font[67], 2, 1, font[199 - 32], 8, 12, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[199 - 32], 9, 12, 1, 1 );
            fheroes2::Copy( font[67], 2, 1, font[199 - 32], 7, 13, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[199 - 32], 8, 13, 1, 1 );
            fheroes2::Copy( font[67], 3, 0, font[199 - 32], 9, 13, 1, 1 );
            font[199 - 32].setPosition( font[35].x(), font[35].y() );
            updateNormalFontLetterShadow( font[199 - 32] );

            // G with breve.
            font[208 - 32].resize( font[39].width(), font[39].height() + 3 );
            font[208 - 32].reset();
            fheroes2::Copy( font[39], 0, 0, font[208 - 32], 0, 3, font[39].width(), font[39].height() );
            fheroes2::Copy( font[39], 5, 9, font[208 - 32], 5, 0, 7, 2 );
            fheroes2::FillTransform( font[208 - 32], 7, 0, 3, 1, 1 );
            font[208 - 32].setPosition( font[39].x(), font[39].y() - 3 );
            updateNormalFontLetterShadow( font[208 - 32] );

            // O with diaeresis, two dots above.
            font[214 - 32].resize( font[47].width(), font[47].height() + 3 );
            font[214 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[214 - 32], 0, 3, font[47].width(), font[47].height() );
            fheroes2::Copy( font[214 - 32], 1, 2 + 3, font[214 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 2, 2 + 3, font[214 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 5, 0, font[214 - 32], 10, 0, 2, 1 );
            font[214 - 32].setPosition( font[47].x(), font[47].y() - 3 );
            updateNormalFontLetterShadow( font[214 - 32] );

            // U with diaeresis.
            font[220 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[220 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[220 - 32], 0, 3, font[53].width(), font[53].height() );
            fheroes2::Copy( font[220 - 32], 1, 1 + 3, font[220 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 2, 1 + 3, font[220 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 4, 0, font[220 - 32], 9, 0, 2, 1 );
            font[220 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            updateNormalFontLetterShadow( font[220 - 32] );

            // I with dot above.
            font[221 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[221 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[221 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[73], 1, 0, font[221 - 32], 3, 0, 3, 2 );
            font[221 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[221 - 32] );

            // S with cedilla.
            font[222 - 32].resize( font[51].width(), font[51].height() + 3 );
            font[222 - 32].reset();
            fheroes2::Copy( font[51], 0, 0, font[222 - 32], 0, 0, font[51].width(), font[51].height() );
            fheroes2::Copy( font[199 - 32], 7, 11, font[222 - 32], 5, 11, 3, 3 );
            font[222 - 32].setPosition( font[51].x(), font[51].y() );
            updateNormalFontLetterShadow( font[222 - 32] );

            // c with cedilla.
            font[231 - 32].resize( font[67].width(), font[67].height() + 3 );
            font[231 - 32].reset();
            fheroes2::Copy( font[67], 0, 0, font[231 - 32], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[199 - 32], 7, 11, font[231 - 32], 4, 7, 3, 3 );
            font[231 - 32].setPosition( font[67].x(), font[67].y() );
            updateNormalFontLetterShadow( font[231 - 32] );

            // g with breve.
            font[240 - 32].resize( font[71].width(), font[71].height() + 3 );
            font[240 - 32].reset();
            fheroes2::Copy( font[71], 0, 0, font[240 - 32], 0, 3, font[71].width(), font[71].height() );
            fheroes2::Copy( font[79], 2, 5, font[240 - 32], 2, 0, 6, 2 );
            fheroes2::FillTransform( font[240 - 32], 4, 0, 2, 1, 1 );
            font[240 - 32].setPosition( font[71].x(), font[71].y() - 3 );
            updateNormalFontLetterShadow( font[240 - 32] );

            // o with diaeresis, two dots above.
            font[246 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[246 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[246 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[246 - 32], 4, 0 + 3, font[246 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 6, 1 + 3, font[246 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[246 - 32], 6, 1 + 3, font[246 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 4, 1 + 3, font[246 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[246 - 32], 2, 0, font[246 - 32], 6, 0, 2, 2 );
            font[246 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[246 - 32] );

            // u with diaeresis.
            font[252 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[252 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[252 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[252 - 32], 2, 0 + 3, font[252 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 1 + 3, font[252 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 1 + 3, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 3, 6 + 3, font[252 - 32], 2, 1, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 0, font[252 - 32], 6, 0, 2, 2 );
            font[252 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[252 - 32] );

            // i without dot above.
            font[253 - 32] = font[73];
            fheroes2::FillTransform( font[253 - 32], 0, 0, font[253 - 32].width(), 3, 1 );
            fheroes2::Copy( font[83], 2, 0, font[253 - 32], 0, 3, 1, 1 );
            updateNormalFontLetterShadow( font[253 - 32] );

            // s with cedilla.
            font[254 - 32].resize( font[83].width(), font[83].height() + 3 );
            font[254 - 32].reset();
            fheroes2::Copy( font[83], 0, 0, font[254 - 32], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[199 - 32], 7, 11, font[254 - 32], 4, 7, 3, 3 );
            font[254 - 32].setPosition( font[83].x(), font[83].y() );
            updateNormalFontLetterShadow( font[254 - 32] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // C with cedilla.
            font[199 - 32].resize( font[35].width(), font[35].height() + 3 );
            font[199 - 32].reset();
            fheroes2::Copy( font[35], 0, 0, font[199 - 32], 0, 0, font[35].width(), font[35].height() );
            fheroes2::Copy( font[35], 1, 1, font[199 - 32], 3, 7, 2, 2 );
            font[199 - 32].setPosition( font[35].x(), font[35].y() );
            updateSmallFontLetterShadow( font[199 - 32] );

            // G with breve.
            font[208 - 32].resize( font[39].width(), font[39].height() + 3 );
            font[208 - 32].reset();
            fheroes2::Copy( font[39], 0, 0, font[208 - 32], 0, 3, font[39].width(), font[39].height() );
            fheroes2::Copy( font[35], 2, 5, font[208 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[35], 2, 5, font[208 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[35], 3, 6, font[208 - 32], 4, 1, 2, 1 );
            font[208 - 32].setPosition( font[39].x(), font[39].y() - 3 );
            updateSmallFontLetterShadow( font[208 - 32] );

            // O with diaeresis, two dots above.
            font[214 - 32].resize( font[47].width(), font[47].height() + 2 );
            font[214 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[214 - 32], 0, 2, font[47].width(), font[47].height() );
            fheroes2::Copy( font[214 - 32], 3, 0 + 2, font[214 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 3, 0 + 2, font[214 - 32], 5, 0, 1, 1 );
            font[214 - 32].setPosition( font[47].x(), font[47].y() - 2 );
            updateSmallFontLetterShadow( font[214 - 32] );

            // U with diaeresis.
            font[220 - 32].resize( font[53].width(), font[53].height() + 2 );
            font[220 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[220 - 32], 0, 2, font[53].width(), font[53].height() );
            fheroes2::Copy( font[220 - 32], 3, 0 + 2, font[220 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 3, 0 + 2, font[220 - 32], 6, 0, 1, 1 );
            font[220 - 32].setPosition( font[53].x(), font[53].y() - 2 );
            updateSmallFontLetterShadow( font[220 - 32] );

            // I with dot above.
            font[221 - 32].resize( font[41].width(), font[41].height() + 2 );
            font[221 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[221 - 32], 0, 2, font[41].width(), font[41].height() );
            fheroes2::Copy( font[41], 2, 0, font[221 - 32], 2, 0, 2, 1 );
            font[221 - 32].setPosition( font[41].x(), font[41].y() - 2 );
            updateSmallFontLetterShadow( font[221 - 32] );

            // S with cedilla.
            font[222 - 32].resize( font[51].width(), font[51].height() + 3 );
            font[222 - 32].reset();
            fheroes2::Copy( font[51], 0, 0, font[222 - 32], 0, 0, font[51].width(), font[51].height() );
            fheroes2::Copy( font[35], 1, 1, font[222 - 32], 3, 7, 2, 2 );
            font[222 - 32].setPosition( font[51].x(), font[51].y() );
            updateSmallFontLetterShadow( font[222 - 32] );

            // c with cedilla.
            font[231 - 32].resize( font[67].width(), font[67].height() + 3 );
            font[231 - 32].reset();
            fheroes2::Copy( font[67], 0, 0, font[231 - 32], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[199 - 32], 1, 1, font[231 - 32], 2, 5, 2, 2 );
            font[231 - 32].setPosition( font[67].x(), font[67].y() );
            updateSmallFontLetterShadow( font[231 - 32] );

            // g with breve.
            font[240 - 32].resize( font[71].width(), font[71].height() + 3 );
            font[240 - 32].reset();
            fheroes2::Copy( font[71], 0, 0, font[240 - 32], 0, 3, font[71].width(), font[71].height() );
            fheroes2::Copy( font[35], 2, 5, font[240 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[35], 2, 5, font[240 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[35], 3, 6, font[240 - 32], 3, 1, 1, 1 );
            font[240 - 32].setPosition( font[71].x(), font[71].y() - 3 );
            updateSmallFontLetterShadow( font[240 - 32] );

            // o with diaeresis, two dots above.
            font[246 - 32].resize( font[79].width(), font[79].height() + 2 );
            font[246 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[246 - 32], 0, 2, font[79].width(), font[79].height() );
            fheroes2::Copy( font[246 - 32], 3, 0 + 2, font[246 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 3, 0 + 2, font[246 - 32], 4, 0, 1, 1 );
            font[246 - 32].setPosition( font[79].x(), font[79].y() - 2 );
            updateSmallFontLetterShadow( font[246 - 32] );

            // u with diaeresis.
            font[252 - 32].resize( font[85].width(), font[85].height() + 2 );
            font[252 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[252 - 32], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 6, 0, 1, 1 );
            font[252 - 32].setPosition( font[85].x(), font[85].y() - 2 );
            updateSmallFontLetterShadow( font[252 - 32] );

            // i without dot above.
            font[253 - 32].resize( font[73].width(), font[73].height() + 1 );
            font[253 - 32].reset();
            fheroes2::Copy( font[73], 0, 2, font[253 - 32], 0, 3, font[73].width(), 6 );
            font[253 - 32].setPosition( font[73].x(), font[73].y() - 1 );
            updateSmallFontLetterShadow( font[253 - 32] );

            // s with cedilla.
            font[254 - 32].resize( font[83].width(), font[83].height() + 3 );
            font[254 - 32].reset();
            fheroes2::Copy( font[83], 0, 0, font[254 - 32], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[35], 1, 1, font[254 - 32], 2, 5, 2, 2 );
            font[254 - 32].setPosition( font[83].x(), font[83].y() );
            updateSmallFontLetterShadow( font[254 - 32] );
        }
    }

    void generateCP1258Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );

            const fheroes2::Sprite firstSprite{ icnVsSprite[icnId][0] };
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, firstSprite );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // A with grave accent ` and generate the grave accent for further use.
            font[192 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[192 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[192 - 32], 0, 3, font[33].width(), font[33].height() );
            font[192 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            fheroes2::Copy( font[192 - 32], 3, 4, font[192 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[192 - 32], 3, 4, font[192 - 32], 8, 1, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[192 - 32], 3, 3, font[192 - 32], 10, 1, 1, 1 );
            updateNormalFontLetterShadow( font[192 - 32] );

            // A with acute accent. Generate the accent for further use.
            font[193 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[193 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[193 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[193 - 32], 3, 4, font[193 - 32], 10, 0, 1, 1 );
            fheroes2::Copy( font[193 - 32], 4, 4, font[193 - 32], 9, 0, 1, 1 );
            fheroes2::Copy( font[193 - 32], 3, 4, font[193 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[193 - 32], 4, 4, font[193 - 32], 8, 1, 1, 1 );
            fheroes2::Copy( font[193 - 32], 3, 3, font[193 - 32], 7, 1, 1, 1 );
            font[193 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateNormalFontLetterShadow( font[193 - 32] );

            // A with circumflex accent. Generation of accent for further use.
            font[194 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[194 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 7, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 7, 1, font[194 - 32], 9, 1, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 0, font[194 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[97 - 32], 1, 0, font[194 - 32], 9, 0, 1, 1 );
            font[194 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[194 - 32] );

            // A with breve and generate the accent for further use.
            font[195 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[195 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[195 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[39], 5, 9, font[195 - 32], 5, 0, 7, 2 );
            fheroes2::FillTransform( font[195 - 32], 7, 0, 3, 1, 1 );
            font[195 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateNormalFontLetterShadow( font[195 - 32] );

            // E with grave accent `.
            font[200 - 32].resize( font[37].width(), font[37].height() + 3 );
            font[200 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[200 - 32], 0, 3, font[37].width(), font[37].height() );
            font[200 - 32].setPosition( font[37].x(), font[37].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[200 - 32], 4, 0, 4, 2 );
            updateNormalFontLetterShadow( font[200 - 32] );

            // E with acute accent.
            font[201 - 32].resize( font[37].width(), font[37].height() + 3 );
            font[201 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[201 - 32], 0, 3, font[37].width(), font[37].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[201 - 32], 5, 0, 4, 2 );
            font[201 - 32].setPosition( font[37].x(), font[37].y() - 3 );
            updateNormalFontLetterShadow( font[201 - 32] );

            // E with circumflex accent.
            font[202 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[202 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[202 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[202 - 32], 5, 0, 3, 2 );
            font[202 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[202 - 32] );

            // [204 - 32] COMBINING GRAVE TONE MARK `.

            // I with accute accent.
            font[205 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[205 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[205 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[205 - 32], 3, 0, 4, 2 );
            font[205 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[205 - 32] );

            // D with stroke. Needs stroke generated.
            font[208 - 32] = font[36];

            // [210 - 32] COMBINING HOOK ABOVE.

            // O with acute accent.
            font[211 - 32].resize( font[47].width(), font[47].height() + 3 );
            font[211 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[211 - 32], 0, 3, font[47].width(), font[47].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[211 - 32], 7, 0, 4, 2 );
            font[211 - 32].setPosition( font[47].x(), font[47].y() - 3 );
            updateNormalFontLetterShadow( font[211 - 32] );

            // O with circumflex accent.
            font[212 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[212 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[212 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[212 - 32], 7, 0, 3, 2 );
            font[212 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[212 - 32] );

            // O with horn. Needs accent generation.
            font[213 - 32] = font[79 - 32];

            // U with grave accent `.
            font[217 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[217 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[217 - 32], 0, 3, font[53].width(), font[53].height() );
            font[217 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[217 - 32], 5, 0, 4, 2 );
            updateNormalFontLetterShadow( font[217 - 32] );

            // U with acute accent.
            font[218 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[218 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[218 - 32], 0, 3, font[53].width(), font[53].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[218 - 32], 5, 0, 4, 2 );
            font[218 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            updateNormalFontLetterShadow( font[218 - 32] );

            // U with horn. Needs accent generated.
            font[221 - 32] = font[53];

            // [222 - 32] COMBINING TILDE ~.

            // a with grave accent `.
            font[224 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[224 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[224 - 32], 0, 3, font[65].width(), font[65].height() );
            font[224 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[224 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[224 - 32] );

            // a with acute accent.
            font[225 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[225 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[225 - 32], 3, 0, 4, 2 );
            font[225 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[225 - 32] );

            // a with circumflex accent.
            font[226 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[226 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[226 - 32], 3, 0, 3, 2 );
            font[226 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[226 - 32] );

            // a with breve.
            font[227 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[227 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[227 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 2, 0, font[227 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[65], 2, 0, font[227 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[227 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[65], 2, 0, font[227 - 32], 4, 1, 2, 1 );
            fheroes2::Copy( font[65], 1, 0, font[227 - 32], 6, 1, 1, 1 );
            font[227 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[227 - 32] );

            // e with grave accent `.
            font[232 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[232 - 32], 0, 3, font[69].width(), font[69].height() );
            font[232 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[232 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[232 - 32] );

            // e with acute accent.
            font[233 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[233 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[233 - 32], 3, 0, 4, 2 );
            font[233 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[233 - 32] );

            // e with circumflex accent.
            font[234 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[234 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[234 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[234 - 32], 4, 0, 3, 2 );
            font[234 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[234 - 32] );

            // [236 - 32] COMBINING ACUTE TONE MARK.

            // i with acute accent.
            font[237 - 32] = font[73];
            fheroes2::FillTransform( font[237 - 32], 0, 0, font[237 - 32].width(), 3, 1 );
            fheroes2::Copy( font[193 - 32], 7, 0, font[237 - 32], 1, 0, 4, 2 );
            updateNormalFontLetterShadow( font[237 - 32] );

            // d with stroke. Needs stroke generated.
            font[240 - 32] = font[68];

            // [242 - 32] COMBINING DOT BELOW.

            // o with acute accent.
            font[243 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[243 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[243 - 32], 3, 0, 4, 2 );
            font[243 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[243 - 32] );

            // o with circumflex accent.
            font[244 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[244 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[244 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[194 - 32], 7, 0, font[244 - 32], 4, 0, 3, 2 );
            font[244 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[244 - 32] );

            // o with horn. Needs accent generated.
            font[245 - 32] = font[111 - 32];

            // u with grave accent `.
            font[249 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[249 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[249 - 32], 0, 3, font[85].width(), font[85].height() );
            font[249 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[249 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[249 - 32] );

            // u with acute accent.
            font[250 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[250 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[250 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[250 - 32], 3, 0, 4, 2 );
            font[250 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[250 - 32] );

            // u with horn. Needs accent generated
            font[253 - 32] = font[85];
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // A with grave accent `. Generate grave accent for further use.
            font[192 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[192 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[192 - 32], 0, 4, font[33].width(), font[33].height() );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[192 - 32], 4, 4, font[192 - 32], 6, 2, 1, 1 );
            font[192 - 32].setPosition( font[33].x(), font[33].y() - 4 );
            updateSmallFontLetterShadow( font[192 - 32] );

            // A with acute accent. Generate acute accent for further use
            font[193 - 32].resize( font[33].width(), font[33].height() + 4 );
            font[193 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[193 - 32], 0, 4, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 4, 0, font[193 - 32], 4, 2, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[193 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[193 - 32], 6, 0, 1, 1 );
            font[193 - 32].setPosition( font[33].x(), font[33].y() - 4 );
            updateSmallFontLetterShadow( font[193 - 32] );

            // A with circumflex accent. Generate accent for further use.
            font[194 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[194 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 6, 1, 1, 1 );
            font[194 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateSmallFontLetterShadow( font[194 - 32] );

            // A with breve. Generate accent for further use.
            font[195 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[195 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[195 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 3, 0, font[195 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[195 - 32], 4, 1, 2, 1 );
            fheroes2::Copy( font[33], 3, 0, font[195 - 32], 6, 0, 1, 1 );
            font[195 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateSmallFontLetterShadow( font[195 - 32] );

            // E with grave accent `.
            font[200 - 32].resize( font[37].width(), font[37].height() + 4 );
            font[200 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[200 - 32], 0, 4, font[37].width(), font[37].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[200 - 32], 4, 0, 3, 3 );
            font[200 - 32].setPosition( font[37].x(), font[37].y() - 4 );
            updateSmallFontLetterShadow( font[200 - 32] );

            // E with acute accent.
            font[201 - 32].resize( font[37].width(), font[37].height() + 4 );
            font[201 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[201 - 32], 0, 4, font[37].width(), font[37].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[201 - 32], 3, 0, 3, 3 );
            font[201 - 32].setPosition( font[37].x(), font[37].y() - 4 );
            updateSmallFontLetterShadow( font[201 - 32] );

            // E with circumflex accent.
            font[202 - 32].resize( font[69 - 32].width(), font[69 - 32].height() + 3 );
            font[202 - 32].reset();
            fheroes2::Copy( font[69 - 32], 0, 0, font[202 - 32], 0, 3, font[69 - 32].width(), font[69 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[202 - 32], 3, 0, 3, 2 );
            font[202 - 32].setPosition( font[69 - 32].x(), font[69 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[202 - 32] );

            // [204 - 32] COMBINING GRAVE TONE MARK `.

            // I with acute accent.
            font[205 - 32].resize( font[41].width(), font[41].height() + 4 );
            font[205 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[205 - 32], 0, 4, font[41].width(), font[41].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[205 - 32], 1, 0, 3, 3 );
            font[205 - 32].setPosition( font[41].x(), font[41].y() - 4 );
            updateSmallFontLetterShadow( font[205 - 32] );

            // D with stroke. Needs stroke generated.
            font[208 - 32] = font[36];

            // [210 - 32] COMBINING HOOK ABOVE.

            // O with acute accent.
            font[211 - 32].resize( font[47].width(), font[47].height() + 4 );
            font[211 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[211 - 32], 0, 4, font[47].width(), font[47].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[211 - 32], 3, 0, 3, 3 );
            font[211 - 32].setPosition( font[47].x(), font[47].y() - 4 );
            updateSmallFontLetterShadow( font[211 - 32] );

            // O with circumflex accent.
            font[212 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[212 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[212 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[212 - 32], 3, 0, 3, 2 );
            font[212 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[212 - 32] );

            // O with horn. Needs accent.
            font[213 - 32] = font[79 - 32];

            // U with grave accent `.
            font[217 - 32].resize( font[53].width(), font[53].height() + 4 );
            font[217 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[217 - 32], 0, 4, font[53].width(), font[53].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[217 - 32], 4, 0, 3, 3 );
            font[217 - 32].setPosition( font[53].x(), font[53].y() - 4 );
            updateSmallFontLetterShadow( font[217 - 32] );

            // U with acute accent.
            font[218 - 32].resize( font[53].width(), font[53].height() + 4 );
            font[218 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[218 - 32], 0, 4, font[53].width(), font[53].height() );
            fheroes2::Copy( font[193 - 32], 4, 0, font[218 - 32], 4, 0, 3, 3 );
            font[218 - 32].setPosition( font[53].x(), font[53].y() - 4 );
            updateSmallFontLetterShadow( font[218 - 32] );

            // U with horn. Needs accent generated.
            font[221 - 32] = font[53];

            // [222 - 32] COMBINING TILDE ~.

            // a with grave accent `.
            font[224 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[224 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[224 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[224 - 32], 3, 0, 3, 2 );
            font[224 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[224 - 32] );

            // a with acute accent.
            font[225 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[225 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[225 - 32], 3, 0, 2, 2 );
            font[225 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[225 - 32] );

            // a with circumflex accent.
            font[226 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[226 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[226 - 32], 2, 0, 3, 2 );
            font[226 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[226 - 32] );

            // a with breve.
            font[227 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[227 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[227 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[227 - 32], 2, 0, 4, 2 );
            font[227 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[227 - 32] );

            // e with grave accent `.
            font[232 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[232 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[232 - 32], 3, 0, 3, 2 );
            font[232 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[232 - 32] );

            // e with acute accent.
            font[233 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[233 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[233 - 32], 3, 0, 2, 2 );
            font[233 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateSmallFontLetterShadow( font[233 - 32] );

            // e with circumflex accent.
            font[234 - 32].resize( font[101 - 32].width(), font[101 - 32].height() + 3 );
            font[234 - 32].reset();
            fheroes2::Copy( font[101 - 32], 0, 0, font[234 - 32], 0, 3, font[101 - 32].width(), font[101 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[234 - 32], 2, 0, 3, 2 );
            font[234 - 32].setPosition( font[101 - 32].x(), font[101 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[234 - 32] );

            // [236 - 32] COMBINING ACUTE TONE MARK.

            // i with acute accent.
            font[237 - 32].resize( font[73].width(), font[73].height() + 1 );
            font[237 - 32].reset();
            fheroes2::Copy( font[73], 0, 2, font[237 - 32], 0, 3, font[73].width(), 6 );
            fheroes2::Copy( font[193 - 32], 5, 0, font[237 - 32], 1, 0, 2, 2 );
            font[237 - 32].setPosition( font[73].x(), font[73].y() - 1 );
            updateSmallFontLetterShadow( font[237 - 32] );

            // d with stroke. Needs stroke generated.
            font[240 - 32] = font[68];

            // [242 - 32] COMBINING DOT BELOW.

            // o with acute accent.
            font[243 - 32].resize( font[79].width(), font[79].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[243 - 32], 0, 3, font[79].width(), font[79].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[243 - 32], 3, 0, 2, 2 );
            font[243 - 32].setPosition( font[79].x(), font[79].y() - 3 );
            updateSmallFontLetterShadow( font[243 - 32] );

            // o with circumflex accent.
            font[244 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[244 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[244 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[244 - 32], 2, 0, 3, 2 );
            font[244 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[244 - 32] );

            // o with horn. Needs accent to be generated.
            font[245 - 32] = font[111 - 32];

            // u with grave accent `.
            font[249 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[249 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[249 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[192 - 32], 4, 0, font[249 - 32], 3, 0, 3, 2 );
            font[249 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[249 - 32] );

            // u with acute accent.
            font[250 - 32].resize( font[85].width(), font[85].height() + 3 );
            font[250 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[250 - 32], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[193 - 32], 5, 0, font[250 - 32], 3, 0, 2, 2 );
            font[250 - 32].setPosition( font[85].x(), font[85].y() - 3 );
            updateSmallFontLetterShadow( font[250 - 32] );

            // u with horn.
            font[253 - 32] = font[85];
        }
    }

    void generateISO8859_16Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );

            const fheroes2::Sprite firstSprite{ icnVsSprite[icnId][0] };
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, firstSprite );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // S with comma.
            font[170 - 32].resize( font[51].width(), font[51].height() + 4 );
            font[170 - 32].reset();
            fheroes2::Copy( font[51], 0, 0, font[170 - 32], 0, 0, font[51].width(), font[51].height() );
            fheroes2::Copy( font[12], 0, 0, font[170 - 32], 3, 12, font[12].width(), font[12].height() );
            font[170 - 32].setPosition( font[51].x(), font[51].y() );
            updateNormalFontLetterShadow( font[170 - 32] );

            // A with circumflex and generate the accent for further use.
            font[194 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[194 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 7, 0, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 6, 1, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 10, 0, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 11, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 7, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 10, 1, 1, 1 );
            fheroes2::Copy( font[33], 4, 0, font[194 - 32], 9, 0, 1, 1 );
            font[194 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateNormalFontLetterShadow( font[194 - 32] );

            // A with breve and generate the accent for further use.
            font[195 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[195 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[195 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[39], 5, 9, font[195 - 32], 5, 0, 7, 2 );
            fheroes2::FillTransform( font[195 - 32], 7, 0, 3, 1, 1 );
            font[195 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateNormalFontLetterShadow( font[195 - 32] );

            // I with circumflex
            font[206 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[206 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[206 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[194 - 32], 6, 0, font[206 - 32], 1, 0, 6, 2 );
            font[206 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[206 - 32] );

            // T with comma.
            font[222 - 32].resize( font[52].width(), font[52].height() + 4 );
            font[222 - 32].reset();
            fheroes2::Copy( font[52], 0, 0, font[222 - 32], 0, 0, font[52].width(), font[52].height() );
            fheroes2::Copy( font[12], 0, 0, font[222 - 32], 3, 12, font[12].width(), font[12].height() );
            font[222 - 32].setPosition( font[52].x(), font[52].y() );
            updateNormalFontLetterShadow( font[222 - 32] );

            // s with comma.
            font[186 - 32].resize( font[83].width(), font[83].height() + 4 );
            font[186 - 32].reset();
            fheroes2::Copy( font[83], 0, 0, font[186 - 32], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[12], 0, 0, font[186 - 32], 2, 8, font[12].width(), font[12].height() );
            font[186 - 32].setPosition( font[83].x(), font[83].y() );
            updateNormalFontLetterShadow( font[186 - 32] );

            // a with circumflex.
            font[226 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[226 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[194 - 32], 6, 0, font[226 - 32], 2, 0, 3, 2 );
            fheroes2::Copy( font[194 - 32], 10, 0, font[226 - 32], 5, 0, 2, 2 );
            font[226 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[226 - 32] );

            // a with breve.
            font[227 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[227 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[227 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[65], 2, 0, font[227 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[65], 2, 0, font[227 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[65], 1, 0, font[227 - 32], 3, 1, 1, 1 );
            fheroes2::Copy( font[65], 2, 0, font[227 - 32], 4, 1, 2, 1 );
            fheroes2::Copy( font[65], 1, 0, font[227 - 32], 6, 1, 1, 1 );
            font[227 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[227 - 32] );

            // i with circumflex
            font[238 - 32].resize( font[73].width(), font[73].height() );
            font[238 - 32].reset();
            fheroes2::Copy( font[73], 0, 0, font[238 - 32], 0, 0, font[73].width(), font[73].height() );
            fheroes2::Copy( font[226 - 32], 3, 0, font[238 - 32], 1, 0, 3, 2 );
            font[238 - 32].setPosition( font[73].x(), font[73].y() );
            updateNormalFontLetterShadow( font[238 - 32] );

            // t with comma.
            font[254 - 32].resize( font[84].width(), font[84].height() + 4 );
            font[254 - 32].reset();
            fheroes2::Copy( font[84], 0, 0, font[254 - 32], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[12], 0, 0, font[254 - 32], 1, 12, font[12].width(), font[12].height() );
            font[254 - 32].setPosition( font[84].x(), font[84].y() );
            updateNormalFontLetterShadow( font[254 - 32] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // S with comma.
            font[170 - 32].resize( font[51].width(), font[51].height() + 4 );
            font[170 - 32].reset();
            fheroes2::Copy( font[51], 0, 0, font[170 - 32], 0, 0, font[51].width(), font[51].height() );
            fheroes2::Copy( font[12], 0, 0, font[170 - 32], 2, 8, font[12].width(), font[12].height() );
            font[170 - 32].setPosition( font[51].x(), font[51].y() );
            updateSmallFontLetterShadow( font[170 - 32] );

            // A with circumflex and generate the accent for further use.
            font[194 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[194 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[194 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 4, 1, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[194 - 32], 6, 1, 1, 1 );
            font[194 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateSmallFontLetterShadow( font[194 - 32] );

            // A with breve and generate the accent for further use.
            font[195 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[195 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[195 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[33], 3, 0, font[195 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[33], 3, 0, font[195 - 32], 4, 1, 2, 1 );
            fheroes2::Copy( font[33], 3, 0, font[195 - 32], 6, 0, 1, 1 );
            font[195 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            updateSmallFontLetterShadow( font[195 - 32] );

            // I with circumflex
            font[206 - 32].resize( font[41].width(), font[41].height() + 3 );
            font[206 - 32].reset();
            fheroes2::Copy( font[41], 0, 0, font[206 - 32], 0, 3, font[41].width(), font[41].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[206 - 32], 1, 0, 3, 2 );
            font[206 - 32].setPosition( font[41].x(), font[41].y() - 3 );
            updateNormalFontLetterShadow( font[206 - 32] );

            // T with comma.
            font[222 - 32].resize( font[52].width(), font[52].height() + 4 );
            font[222 - 32].reset();
            fheroes2::Copy( font[52], 0, 0, font[222 - 32], 0, 0, font[52].width(), font[52].height() );
            fheroes2::Copy( font[12], 0, 0, font[222 - 32], 2, 8, font[12].width(), font[12].height() );
            font[222 - 32].setPosition( font[52].x(), font[52].y() );
            updateSmallFontLetterShadow( font[222 - 32] );

            // s with comma.
            font[186 - 32].resize( font[83].width(), font[83].height() + 4 );
            font[186 - 32].reset();
            fheroes2::Copy( font[83], 0, 0, font[186 - 32], 0, 0, font[83].width(), font[83].height() );
            fheroes2::Copy( font[12], 0, 0, font[186 - 32], 1, 6, font[12].width(), font[12].height() );
            font[186 - 32].setPosition( font[83].x(), font[83].y() );
            updateSmallFontLetterShadow( font[186 - 32] );

            // a with circumflex.
            font[226 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[226 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[226 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[194 - 32], 4, 0, font[226 - 32], 2, 0, 3, 2 );
            font[226 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[226 - 32] );

            // a with breve.
            font[227 - 32].resize( font[65].width(), font[65].height() + 3 );
            font[227 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[227 - 32], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[195 - 32], 3, 0, font[227 - 32], 2, 0, 4, 2 );
            font[227 - 32].setPosition( font[65].x(), font[65].y() - 3 );
            updateSmallFontLetterShadow( font[227 - 32] );

            // i with circumflex
            font[238 - 32].resize( font[73].width(), font[73].height() + 1 );
            font[238 - 32].reset();
            fheroes2::Copy( font[73], 0, 0, font[238 - 32], 0, 1, font[73].width(), font[73].height() );
            fheroes2::Copy( font[226 - 32], 2, 0, font[238 - 32], 1, 0, 3, 2 );
            font[238 - 32].setPosition( font[73].x(), font[73].y() - 1 );
            updateNormalFontLetterShadow( font[238 - 32] );

            // t with comma.
            font[254 - 32].resize( font[84].width(), font[84].height() + 4 );
            font[254 - 32].reset();
            fheroes2::Copy( font[84], 0, 0, font[254 - 32], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[12], 0, 0, font[254 - 32], 1, 8, font[12].width(), font[12].height() );
            font[254 - 32].setPosition( font[84].x(), font[84].y() );
            updateSmallFontLetterShadow( font[254 - 32] );
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

        released.resize( baseFontSize );

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
        released[1].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[13].setPosition( buttonFontOffset.x - 2, buttonFontOffset.y );
        released[14].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[26].setPosition( buttonFontOffset.x - 2, buttonFontOffset.y );
        released[27].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[31].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[33].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[54].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[57].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );

        // !
        released[1].resize( 2 + offset * 2, 10 + offset * 2 );
        released[1].reset();
        fheroes2::DrawLine( released[1], { offset + 0, offset + 0 }, { offset + 0, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[1], { offset + 1, offset + 0 }, { offset + 1, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[1], { offset + 0, offset + 8 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[1], { offset + 1, offset + 8 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );

        // "
        released[2].resize( 6 + offset * 2, 10 + offset * 2 );
        released[2].reset();
        fheroes2::DrawLine( released[2], { offset + 1, offset + 0 }, { offset + 1, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[2], { offset + 5, offset + 0 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[2], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[2], offset + 4, offset + 0, buttonGoodReleasedColor );

        // #
        released[3].resize( 10 + offset * 2, 10 + offset * 2 );
        released[3].reset();
        fheroes2::DrawLine( released[3], { offset + 1, offset + 3 }, { offset + 9, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[3], { offset + 0, offset + 6 }, { offset + 8, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[3], { offset + 4, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[3], { offset + 7, offset + 0 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // %
        released[5].resize( 9 + offset * 2, 10 + offset * 2 );
        released[5].reset();
        fheroes2::DrawLine( released[5], { offset + 6, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 1, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 1, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 3, offset + 1 }, { offset + 3, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 7, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 7, offset + 6 }, { offset + 6, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 8, offset + 8 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[5], { offset + 5, offset + 8 }, { offset + 5, offset + 7 }, buttonGoodReleasedColor );

        // &
        released[6].resize( 8 + offset * 2, 10 + offset * 2 );
        released[6].reset();
        fheroes2::DrawLine( released[6], { offset + 2, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 1, offset + 1 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 4, offset + 1 }, { offset + 4, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 2, offset + 3 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 3, offset + 3 }, { offset + 1, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 0, offset + 6 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 1, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[6], { offset + 5, offset + 8 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );

        // '
        released[7].resize( 2 + offset * 2, 10 + offset * 2 );
        released[7].reset();
        fheroes2::DrawLine( released[7], { offset + 1, offset + 0 }, { offset + 1, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[7], offset + 0, offset + 0, buttonGoodReleasedColor );

        // (
        released[8].resize( 3 + offset * 2, 10 + offset * 2 );
        released[8].reset();
        fheroes2::DrawLine( released[8], { offset + 0, offset + 3 }, { offset + 0, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[8], { offset + 1, offset + 1 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[8], { offset + 1, offset + 7 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[8], offset + 2, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[8], offset + 2, offset + 9, buttonGoodReleasedColor );

        // )
        released[9].resize( 3 + offset * 2, 10 + offset * 2 );
        released[9].reset();
        fheroes2::DrawLine( released[9], { offset + 2, offset + 3 }, { offset + 2, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[9], { offset + 1, offset + 1 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[9], { offset + 1, offset + 7 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[9], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[9], offset + 0, offset + 9, buttonGoodReleasedColor );

        //*
        released[10].resize( 5 + offset * 2, 10 + offset * 2 );
        released[10].reset();
        fheroes2::DrawLine( released[10], { offset + 2, offset + 0 }, { offset + 2, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[10], { offset + 0, offset + 1 }, { offset + 4, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[10], { offset + 0, offset + 4 }, { offset + 4, offset + 1 }, buttonGoodReleasedColor );

        // +
        released[11].resize( 5 + offset * 2, 10 + offset * 2 );
        released[11].reset();
        fheroes2::DrawLine( released[11], { offset + 0, offset + 5 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[11], { offset + 2, offset + 3 }, { offset + 2, offset + 7 }, buttonGoodReleasedColor );

        // ,
        released[12].resize( 3 + offset * 2, 11 + offset * 2 );
        released[12].reset();
        fheroes2::DrawLine( released[12], { offset + 1, offset + 8 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[12], { offset + 1, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[12], { offset + 0, offset + 10 }, { offset + 1, offset + 10 }, buttonGoodReleasedColor );

        // -
        released[13].resize( 6 + offset * 2, 6 + offset * 2 );
        released[13].reset();
        fheroes2::DrawLine( released[13], { offset + 0, offset + 5 }, { offset + 5, offset + 5 }, buttonGoodReleasedColor );

        // .
        released[14].resize( 2 + offset * 2, 10 + offset * 2 );
        released[14].reset();
        fheroes2::DrawLine( released[14], { offset + 0, offset + 8 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[14], { offset + 0, offset + 9 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );

        // /
        released[15].resize( 4 + offset * 2, 10 + offset * 2 );
        released[15].reset();
        fheroes2::DrawLine( released[15], { offset + 3, offset + 0 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );

        // 0
        released[16].resize( 9 + offset * 2, 10 + offset * 2 );
        released[16].reset();
        fheroes2::DrawLine( released[16], { offset + 2, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[16], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[16], { offset + 2, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[16], { offset + 8, offset + 2 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[16], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[16], offset + 7, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[16], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[16], offset + 7, offset + 8, buttonGoodReleasedColor );

        // 1
        released[17].resize( 5 + offset * 2, 10 + offset * 2 );
        released[17].reset();
        fheroes2::DrawLine( released[17], { offset + 2, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[17], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[17], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[17], offset + 0, offset + 2, buttonGoodReleasedColor );

        // 2
        released[18].resize( 7 + offset * 2, 10 + offset * 2 );
        released[18].reset();
        fheroes2::DrawLine( released[18], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[18], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[18], { offset + 5, offset + 4 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[18], { offset + 1, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[18], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[18], offset + 6, offset + 8, buttonGoodReleasedColor );

        // 3
        released[19].resize( 7 + offset * 2, 10 + offset * 2 );
        released[19].reset();
        fheroes2::DrawLine( released[19], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 2, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[19], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[19], offset + 0, offset + 8, buttonGoodReleasedColor );

        // 4
        released[20].resize( 8 + offset * 2, 10 + offset * 2 );
        released[20].reset();
        fheroes2::DrawLine( released[20], { offset + 0, offset + 4 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[20], { offset + 5, offset + 0 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[20], { offset + 5, offset + 6 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[20], { offset + 0, offset + 5 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[20], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );

        // 5
        released[21].resize( 7 + offset * 2, 10 + offset * 2 );
        released[21].reset();
        fheroes2::DrawLine( released[21], { offset + 0, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[21], { offset + 0, offset + 1 }, { offset + 0, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[21], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[21], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[21], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[21], offset + 0, offset + 8, buttonGoodReleasedColor );

        // 6
        released[22].resize( 7 + offset * 2, 10 + offset * 2 );
        released[22].reset();
        fheroes2::DrawLine( released[22], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[22], { offset + 0, offset + 1 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[22], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[22], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[22], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[22], offset + 6, offset + 1, buttonGoodReleasedColor );

        // 7
        released[23].resize( 7 + offset * 2, 10 + offset * 2 );
        released[23].reset();
        fheroes2::DrawLine( released[23], { offset + 0, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[23], { offset + 6, offset + 0 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[23], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[23], offset + 1, offset + 9, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[23], offset + 3, offset + 9, buttonGoodReleasedColor );

        // 8
        released[24].resize( 7 + offset * 2, 10 + offset * 2 );
        released[24].reset();
        fheroes2::DrawLine( released[24], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[24], { offset + 0, offset + 1 }, { offset + 0, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[24], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[24], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[24], { offset + 0, offset + 5 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[24], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[24], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // 9
        released[25].resize( 7 + offset * 2, 10 + offset * 2 );
        released[25].reset();
        fheroes2::DrawLine( released[25], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[25], { offset + 0, offset + 1 }, { offset + 0, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[25], { offset + 6, offset + 1 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[25], { offset + 1, offset + 5 }, { offset + 5, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[25], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[25], offset + 0, offset + 8, buttonGoodReleasedColor );

        // :
        released[26].resize( 2 + offset * 2, 10 + offset * 2 );
        released[26].reset();
        fheroes2::DrawLine( released[26], { offset + 0, offset + 3 }, { offset + 1, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[26], { offset + 0, offset + 4 }, { offset + 1, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[26], { offset + 0, offset + 8 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[26], { offset + 0, offset + 9 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );

        // ;
        released[27].resize( 3 + offset * 2, 11 + offset * 2 );
        released[27].reset();
        fheroes2::DrawLine( released[27], { offset + 1, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[27], { offset + 1, offset + 4 }, { offset + 2, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[27], { offset + 1, offset + 8 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[27], { offset + 1, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[27], { offset + 0, offset + 10 }, { offset + 1, offset + 10 }, buttonGoodReleasedColor );

        // <
        released[28].resize( 4 + offset * 2, 10 + offset * 2 );
        released[28].reset();
        fheroes2::DrawLine( released[28], { offset + 3, offset + 2 }, { offset + 0, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[28], { offset + 3, offset + 8 }, { offset + 1, offset + 6 }, buttonGoodReleasedColor );

        // =
        released[29].resize( 6 + offset * 2, 8 + offset * 2 );
        released[29].reset();
        fheroes2::DrawLine( released[29], { offset + 0, offset + 3 }, { offset + 5, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[29], { offset + 0, offset + 7 }, { offset + 5, offset + 7 }, buttonGoodReleasedColor );

        // >
        released[30].resize( 4 + offset * 2, 10 + offset * 2 );
        released[30].reset();
        fheroes2::DrawLine( released[30], { offset + 0, offset + 2 }, { offset + 3, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[30], { offset + 0, offset + 8 }, { offset + 2, offset + 6 }, buttonGoodReleasedColor );

        // ?
        released[31].resize( 6 + offset * 2, 10 + offset * 2 );
        released[31].reset();
        fheroes2::DrawLine( released[31], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[31], { offset + 1, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[31], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[31], { offset + 4, offset + 3 }, { offset + 3, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[31], { offset + 2, offset + 8 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[31], { offset + 2, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );

        // A
        released[33].resize( 13 + offset * 2, 10 + offset * 2 );
        released[33].reset();
        fheroes2::DrawLine( released[33], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 5, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 2, offset + 8 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[33], { offset + 7, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[33], offset + 4, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[33], offset + 5, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[33], offset + 5, offset + 2, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[33], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[33], offset + 6, offset + 0, buttonGoodReleasedColor );

        // B
        released[34].resize( 11 + offset * 2, 10 + offset * 2 );
        released[34].reset();
        fheroes2::DrawLine( released[34], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[34], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[34], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[34], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[34], { offset + 10, offset + 2 }, { offset + 10, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[34], { offset + 10, offset + 6 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[34], offset + 9, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[34], offset + 9, offset + 1, buttonGoodReleasedColor );

        // C
        released[35].resize( 10 + offset * 2, 10 + offset * 2 );
        released[35].reset();
        fheroes2::DrawLine( released[35], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[35], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[35], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[35], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[35], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[35], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[35], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[35], offset + 8, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[35], offset + 9, offset + 7, buttonGoodReleasedColor );

        // D
        released[36].resize( 11 + offset * 2, 10 + offset * 2 );
        released[36].reset();
        fheroes2::DrawLine( released[36], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[36], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[36], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[36], { offset + 10, offset + 2 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[36], offset + 9, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[36], offset + 9, offset + 8, buttonGoodReleasedColor );

        // E
        released[37].resize( 9 + offset * 2, 10 + offset * 2 );
        released[37].reset();
        fheroes2::DrawLine( released[37], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[37], { offset + 3, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[37], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[37], offset + 8, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[37], offset + 6, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[37], offset + 6, offset + 5, buttonGoodReleasedColor );

        // F
        released[38].resize( 9 + offset * 2, 10 + offset * 2 );
        released[38].reset();
        fheroes2::DrawLine( released[38], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[38], { offset + 3, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[38], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[38], offset + 6, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[38], offset + 6, offset + 5, buttonGoodReleasedColor );

        // G
        released[39].resize( 11 + offset * 2, 10 + offset * 2 );
        released[39].reset();
        fheroes2::DrawLine( released[39], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[39], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[39], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[39], { offset + 7, offset + 5 }, { offset + 10, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[39], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[39], { offset + 9, offset + 6 }, { offset + 9, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[39], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[39], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[39], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[39], offset + 8, offset + 8, buttonGoodReleasedColor );

        // H
        released[40].resize( 13 + offset * 2, 10 + offset * 2 );
        released[40].reset();
        fheroes2::DrawLine( released[40], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 8, offset + 0 }, { offset + 12, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 10, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[40], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );

        // I
        released[41].resize( 5 + offset * 2, 10 + offset * 2 );
        released[41].reset();
        fheroes2::DrawLine( released[41], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[41], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[41], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );

        // J
        released[42].resize( 8 + offset * 2, 10 + offset * 2 );
        released[42].reset();
        fheroes2::DrawLine( released[42], { offset + 3, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[42], { offset + 1, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[42], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[42], { offset + 0, offset + 7 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );

        // K
        released[43].resize( 12 + offset * 2, 10 + offset * 2 );
        released[43].reset();
        fheroes2::DrawLine( released[43], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 3, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 6, offset + 3 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 6, offset + 5 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[43], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );

        // L
        released[44].resize( 9 + offset * 2, 10 + offset * 2 );
        released[44].reset();
        fheroes2::DrawLine( released[44], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[44], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[44], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[44], offset + 8, offset + 8, buttonGoodReleasedColor );

        // M
        released[45].resize( 15 + offset * 2, 10 + offset * 2 );
        released[45].reset();
        fheroes2::DrawLine( released[45], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 2, offset + 0 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 3, offset + 1 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 8, offset + 4 }, { offset + 11, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 12, offset + 1 }, { offset + 12, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 12, offset + 0 }, { offset + 14, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[45], { offset + 10, offset + 9 }, { offset + 14, offset + 9 }, buttonGoodReleasedColor );

        // N
        released[46].resize( 14 + offset * 2, 10 + offset * 2 );
        released[46].reset();
        fheroes2::DrawLine( released[46], { offset + 0, offset + 0 }, { offset + 1, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[46], { offset + 2, offset + 0 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[46], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[46], { offset + 3, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[46], { offset + 9, offset + 0 }, { offset + 13, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[46], { offset + 11, offset + 0 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );

        // O
        released[47].resize( 10 + offset * 2, 10 + offset * 2 );
        released[47].reset();
        fheroes2::DrawLine( released[47], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[47], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[47], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[47], { offset + 9, offset + 2 }, { offset + 9, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[47], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[47], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[47], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[47], offset + 8, offset + 8, buttonGoodReleasedColor );

        // P
        released[48].resize( 11 + offset * 2, 10 + offset * 2 );
        released[48].reset();
        fheroes2::DrawLine( released[48], { offset + 0, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[48], { offset + 10, offset + 1 }, { offset + 10, offset + 4 }, buttonGoodReleasedColor );

        // Q
        released[49].resize( 11 + offset * 2, 11 + offset * 2 );
        released[49].reset();
        fheroes2::DrawLine( released[49], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 9, offset + 2 }, { offset + 9, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 4, offset + 7 }, { offset + 5, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 6, offset + 7 }, { offset + 9, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[49], { offset + 10, offset + 10 }, { offset + 11, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 8, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 8, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[49], offset + 12, offset + 9, buttonGoodReleasedColor );

        // R
        released[50].resize( 12 + offset * 2, 10 + offset * 2 );
        released[50].reset();
        fheroes2::DrawLine( released[50], { offset + 0, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 3, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 10, offset + 1 }, { offset + 10, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[50], { offset + 7, offset + 6 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );

        // S
        released[51].resize( 9 + offset * 2, 10 + offset * 2 );
        released[51].reset();
        fheroes2::DrawLine( released[51], { offset + 1, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 0, offset + 1 }, { offset + 0, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 1, offset + 4 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 8, offset + 5 }, { offset + 8, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 1, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[51], { offset + 0, offset + 8 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[51], offset + 8, offset + 1, buttonGoodReleasedColor );

        // T
        released[52].resize( 11 + offset * 2, 10 + offset * 2 );
        released[52].reset();
        fheroes2::DrawLine( released[52], { offset + 0, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 10, offset + 1 }, { offset + 10, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[52], { offset + 4, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );

        // U
        released[53].resize( 13 + offset * 2, 10 + offset * 2 );
        released[53].reset();
        fheroes2::DrawLine( released[53], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 8, offset + 0 }, { offset + 12, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 2, offset + 1 }, { offset + 2, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 10, offset + 1 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[53], { offset + 4, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[53], offset + 3, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[53], offset + 9, offset + 8, buttonGoodReleasedColor );

        // V
        released[54].resize( 11 + offset * 2, 10 + offset * 2 );
        released[54].reset();
        fheroes2::DrawLine( released[54], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 6, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 2, offset + 1 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[54], { offset + 8, offset + 1 }, { offset + 6, offset + 7 }, buttonGoodReleasedColor );

        // W
        released[55].resize( 17 + offset * 2, 10 + offset * 2 );
        released[55].reset();
        fheroes2::DrawLine( released[55], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 7, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 12, offset + 0 }, { offset + 16, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 2, offset + 1 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 8, offset + 1 }, { offset + 6, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 9, offset + 3 }, { offset + 10, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[55], { offset + 14, offset + 1 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );

        // X
        released[56].resize( 12 + offset * 2, 10 + offset * 2 );
        released[56].reset();
        fheroes2::DrawLine( released[56], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 8, offset + 0 }, { offset + 11, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 2, offset + 1 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[56], { offset + 2, offset + 8 }, { offset + 9, offset + 1 }, buttonGoodReleasedColor );

        // Y
        released[57].resize( 11 + offset * 2, 10 + offset * 2 );
        released[57].reset();
        fheroes2::DrawLine( released[57], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 2, offset + 1 }, { offset + 4, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 6, offset + 3 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 5, offset + 4 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[57], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );

        // Z
        released[58].resize( 9 + offset * 2, 10 + offset * 2 );
        released[58].reset();
        fheroes2::DrawLine( released[58], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[58], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[58], { offset + 7, offset + 1 }, { offset + 0, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[58], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[58], offset + 8, offset + 8, buttonGoodReleasedColor );

        // [
        released[59].resize( 4 + offset * 2, 10 + offset * 2 );
        released[59].reset();
        fheroes2::DrawLine( released[59], { offset + 0, offset + 0 }, { offset + 0, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[59], { offset + 1, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[59], { offset + 1, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );

        /* \ */
        released[60].resize( 4 + offset * 2, 10 + offset * 2 );
        released[60].reset();
        fheroes2::DrawLine( released[60], { offset + 0, offset + 0 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );

        // ]
        released[61].resize( 4 + offset * 2, 10 + offset * 2 );
        released[61].reset();
        fheroes2::DrawLine( released[61], { offset + 3, offset + 0 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[61], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[61], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );

        // ^
        released[62].resize( 5 + offset * 2, 3 + offset * 2 );
        released[62].reset();
        fheroes2::DrawLine( released[62], { offset + 0, offset + 2 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[62], { offset + 3, offset + 1 }, { offset + 4, offset + 2 }, buttonGoodReleasedColor );

        // _
        released[63].resize( 8 + offset * 2, 11 + offset * 2 );
        released[63].reset();
        fheroes2::DrawLine( released[63], { offset + 0, offset + 10 }, { offset + 7, offset + 10 }, buttonGoodReleasedColor );

        // | - replaced with Caps Lock symbol for virtual keyboard
        // TODO: put the Caps Lock symbol to a special font to not replace any other ASCII character.
        released[92].resize( 11 + offset * 2, 11 + offset * 2 );
        released[92].reset();
        fheroes2::SetPixel( released[92], offset + 5, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 4, offset + 1 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 3, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 2, offset + 3 }, { offset + 8, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 1, offset + 4 }, { offset + 9, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 0, offset + 5 }, { offset + 10, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 3, offset + 6 }, { offset + 7, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 3, offset + 7 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[92], { offset + 3, offset + 10 }, { offset + 7, offset + 10 }, buttonGoodReleasedColor );

        // ~ - replaced with Backspace symbol (<x]) for virtual keyboard
        // TODO: put the Backspace symbol to a special font to not replace any other ASCII character.
        released[94].resize( 17 + offset * 2, 9 + offset * 2 );
        released[94].reset();
        fheroes2::SetPixel( released[94], offset + 0, offset + 4, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[94], { offset + 1, offset + 3 }, { offset + 1, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[94], { offset + 2, offset + 2 }, { offset + 2, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[94], { offset + 3, offset + 1 }, { offset + 3, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::Fill( released[94], offset + 4, offset + 0, 13, 9, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[94], { offset + 6, offset + 1 }, { offset + 12, offset + 7 }, 12 );
        fheroes2::DrawLine( released[94], { offset + 7, offset + 1 }, { offset + 13, offset + 7 }, 12 );
        fheroes2::DrawLine( released[94], { offset + 12, offset + 1 }, { offset + 6, offset + 7 }, 12 );
        fheroes2::DrawLine( released[94], { offset + 13, offset + 1 }, { offset + 7, offset + 7 }, 12 );

        // Replaced with Change Language symbol for virtual keyboard
        // TODO: put the Change Language symbol to a special font to not replace any other ASCII character.
        released[95].resize( 14 + offset * 2, 13 + offset * 2 );
        released[95].reset();
        released[95].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        fheroes2::DrawLine( released[95], { offset + 4, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 1, offset + 2 }, { offset + 2, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 1, offset + 3 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 10, offset + 1 }, { offset + 12, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 11, offset + 1 }, { offset + 12, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 0, offset + 4 }, { offset + 13, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 0, offset + 8 }, { offset + 13, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 1, offset + 9 }, { offset + 3, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 1, offset + 10 }, { offset + 2, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 4, offset + 12 }, { offset + 9, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 10, offset + 11 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 11, offset + 11 }, { offset + 12, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 4, offset + 3 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 9, offset + 3 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 8, offset + 1 }, { offset + 8, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 5, offset + 10 }, { offset + 5, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 8, offset + 10 }, { offset + 8, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 0, offset + 5 }, { offset + 0, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[95], { offset + 13, offset + 5 }, { offset + 13, offset + 7 }, buttonGoodReleasedColor );
    }

    void generateCP1250GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), 160, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        for ( const int & charCode : { 138, 140, 141, 142, 143, 175, 192, 193, 194, 195, 196, 197, 198, 200, 201, 203,
                                       204, 205, 206, 207, 209, 210, 211, 212, 213, 214, 216, 218, 219, 220, 221 } ) {
            released[charCode - 32].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        }
        released[217 - 32].setPosition( buttonFontOffset.x, buttonFontOffset.y - 4 );

        // S with caron.
        released[138 - 32].resize( released[83 - 32].width(), released[83 - 32].height() + 3 );
        released[138 - 32].reset();
        fheroes2::Copy( released[83 - 32], 0, 0, released[138 - 32], 0, 3, released[83 - 32].width(), released[83 - 32].height() );
        fheroes2::SetPixel( released[138 - 32], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[138 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // S with acute accent.
        released[140 - 32].resize( released[83 - 32].width(), released[83 - 32].height() + 3 );
        released[140 - 32].reset();
        fheroes2::Copy( released[83 - 32], 0, 0, released[140 - 32], 0, 3, released[83 - 32].width(), released[83 - 32].height() );
        fheroes2::DrawLine( released[140 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // T with caron.
        released[141 - 32].resize( released[84 - 32].width(), released[84 - 32].height() + 3 );
        released[141 - 32].reset();
        fheroes2::Copy( released[84 - 32], 0, 0, released[141 - 32], 0, 3, released[84 - 32].width(), released[84 - 32].height() );
        fheroes2::SetPixel( released[141 - 32], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[141 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // Z with caron.
        released[142 - 32].resize( released[90 - 32].width(), released[90 - 32].height() + 3 );
        released[142 - 32].reset();
        fheroes2::Copy( released[90 - 32], 0, 0, released[142 - 32], 0, 3, released[90 - 32].width(), released[90 - 32].height() );
        fheroes2::SetPixel( released[142 - 32], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[142 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // Z with acute.
        released[143 - 32].resize( released[90 - 32].width(), released[90 - 32].height() + 3 );
        released[143 - 32].reset();
        fheroes2::Copy( released[90 - 32], 0, 0, released[143 - 32], 0, 3, released[90 - 32].width(), released[90 - 32].height() );
        fheroes2::DrawLine( released[143 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // L with stroke.
        released[163 - 32] = released[76 - 32];
        fheroes2::DrawLine( released[131], { offset + 1, offset + 6 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );

        // A with ogonek.
        released[165 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[165 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[165 - 32], 0, 0, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[165 - 32], { offset + 9, offset + 11 }, { offset + 10, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165 - 32], { offset + 10, offset + 12 }, { offset + 11, offset + 12 }, buttonGoodReleasedColor );

        // S with cedilla.
        released[170 - 32].resize( released[83 - 32].width(), released[83 - 32].height() + 3 );
        released[170 - 32].reset();
        fheroes2::Copy( released[83 - 32], 0, 0, released[170 - 32], 0, 0, released[83 - 32].width(), released[83 - 32].height() );
        fheroes2::DrawLine( released[170 - 32], { offset + 4, offset + 10 }, { offset + 5, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[170 - 32], { offset + 3, offset + 12 }, { offset + 4, offset + 12 }, buttonGoodReleasedColor );

        // Z with dot above.
        released[175 - 32].resize( released[90 - 32].width(), released[90 - 32].height() + 3 );
        released[175 - 32].reset();
        fheroes2::Copy( released[90 - 32], 0, 0, released[175 - 32], 0, 3, released[90 - 32].width(), released[90 - 32].height() );
        fheroes2::DrawLine( released[175 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );

        // L with caron.
        released[188 - 32] = released[76 - 32];
        fheroes2::DrawLine( released[188 - 32], { offset + 6, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188 - 32], { offset + 6, offset + 2 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // R with acute.
        released[192 - 32].resize( released[82 - 32].width(), released[82 - 32].height() + 3 );
        released[192 - 32].reset();
        fheroes2::Copy( released[82 - 32], 0, 0, released[192 - 32], 0, 3, released[82 - 32].width(), released[82 - 32].height() );
        fheroes2::DrawLine( released[192 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // A with acute.
        released[193 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[193 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[193 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[193 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // A with circumflex.
        released[194 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[194 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[194 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::SetPixel( released[194 - 32], offset + 5, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[194 - 32], { offset + 6, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // A with breve.
        released[195 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[195 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[195 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[195 - 32], { offset + 5, offset + 0 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195 - 32], { offset + 7, offset + 1 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );

        // A with diaeresis.
        released[196 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[196 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[196 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[196 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[196 - 32], { offset + 8, offset + 1 }, { offset + 9, offset + 1 }, buttonGoodReleasedColor );

        // L with acute.
        released[197 - 32].resize( released[76 - 32].width(), released[76 - 32].height() + 3 );
        released[197 - 32].reset();
        fheroes2::Copy( released[76 - 32], 0, 0, released[197 - 32], 0, 3, released[76 - 32].width(), released[76 - 32].height() );
        fheroes2::DrawLine( released[197 - 32], { offset + 3, offset + 1 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );

        // C with acute accent.
        released[198 - 32].resize( released[67 - 32].width(), released[67 - 32].height() + 3 );
        released[198 - 32].reset();
        fheroes2::Copy( released[67 - 32], 0, 0, released[198 - 32], 0, 3, released[67 - 32].width(), released[67 - 32].height() );
        fheroes2::DrawLine( released[198 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // C with cedilla.
        released[199 - 32].resize( released[67 - 32].width(), released[67 - 32].height() + 3 );
        released[199 - 32].reset();
        fheroes2::Copy( released[67 - 32], 0, 0, released[199 - 32], 0, 0, released[67 - 32].width(), released[67 - 32].height() );
        fheroes2::DrawLine( released[199 - 32], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199 - 32], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );

        // C with caron.
        released[200 - 32].resize( released[67 - 32].width(), released[67 - 32].height() + 3 );
        released[200 - 32].reset();
        fheroes2::Copy( released[67 - 32], 0, 0, released[200 - 32], 0, 3, released[67 - 32].width(), released[67 - 32].height() );
        fheroes2::SetPixel( released[200 - 32], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[200 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // E with acute.
        released[201 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[201 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[201 - 32], 0, 3, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::DrawLine( released[201 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // E with ogonek.
        released[202 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[202 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[202 - 32], 0, 0, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::DrawLine( released[202 - 32], { offset + 6, offset + 11 }, { offset + 7, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[202 - 32], { offset + 7, offset + 12 }, { offset + 8, offset + 12 }, buttonGoodReleasedColor );

        // E with diaeresis.
        released[203 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[203 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[203 - 32], 0, 3, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::DrawLine( released[203 - 32], { offset + 2, offset + 1 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[203 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // E with caron.
        released[204 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[204 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[204 - 32], 0, 3, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::SetPixel( released[204 - 32], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[204 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // I with acute.
        released[205 - 32].resize( released[73 - 32].width(), released[73 - 32].height() + 3 );
        released[205 - 32].reset();
        fheroes2::Copy( released[73 - 32], 0, 0, released[205 - 32], 0, 3, released[73 - 32].width(), released[73 - 32].height() );
        fheroes2::DrawLine( released[205 - 32], { offset + 2, offset + 1 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );

        // I with circumflex.
        released[206 - 32].resize( released[73 - 32].width(), released[73 - 32].height() + 3 );
        released[206 - 32].reset();
        fheroes2::Copy( released[73 - 32], 0, 0, released[206 - 32], 0, 3, released[73 - 32].width(), released[73 - 32].height() );
        fheroes2::SetPixel( released[206 - 32], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[206 - 32], { offset + 2, offset + 0 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );

        // D with caron.
        released[207 - 32].resize( released[68 - 32].width(), released[68 - 32].height() + 3 );
        released[207 - 32].reset();
        fheroes2::Copy( released[68 - 32], 0, 0, released[207 - 32], 0, 3, released[68 - 32].width(), released[68 - 32].height() );
        fheroes2::SetPixel( released[207 - 32], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[207 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // D with stroke.
        released[208 - 32] = released[68 - 32];
        fheroes2::DrawLine( released[208 - 32], { offset + 1, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );

        // N with acute.
        released[209 - 32].resize( released[78 - 32].width(), released[78 - 32].height() + 3 );
        released[209 - 32].reset();
        fheroes2::Copy( released[78 - 32], 0, 0, released[209 - 32], 0, 3, released[78 - 32].width(), released[78 - 32].height() );
        fheroes2::DrawLine( released[209 - 32], { offset + 7, offset + 1 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );

        // N with caron.
        released[210 - 32].resize( released[78 - 32].width(), released[78 - 32].height() + 3 );
        released[210 - 32].reset();
        fheroes2::Copy( released[78 - 32], 0, 0, released[210 - 32], 0, 3, released[78 - 32].width(), released[78 - 32].height() );
        fheroes2::SetPixel( released[210 - 32], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[210 - 32], { offset + 7, offset + 1 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );

        // O with acute.
        released[211 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[211 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[211 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::DrawLine( released[211 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // O with circumflex.
        released[212 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[212 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[212 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::SetPixel( released[212 - 32], offset + 4, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212 - 32], { offset + 5, offset + 0 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );

        // O with double acute.
        released[213 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[213 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[213 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::DrawLine( released[213 - 32], { offset + 3, offset + 1 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[213 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // O with diaeresis.
        released[214 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[214 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[214 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::DrawLine( released[214 - 32], { offset + 2, offset + 1 }, { offset + 3, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[214 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // R with caron.
        released[216 - 32].resize( released[82 - 32].width(), released[82 - 32].height() + 3 );
        released[216 - 32].reset();
        fheroes2::Copy( released[82 - 32], 0, 0, released[216 - 32], 0, 3, released[82 - 32].width(), released[82 - 32].height() );
        fheroes2::SetPixel( released[216 - 32], offset + 5, offset + 0, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[216 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // U with ring above.
        released[217 - 32].resize( released[85 - 32].width(), released[85 - 32].height() + 4 );
        released[217 - 32].reset();
        fheroes2::Copy( released[85 - 32], 0, 0, released[217 - 32], 0, 4, released[85 - 32].width(), released[85 - 32].height() );
        fheroes2::DrawLine( released[217 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[217 - 32], { offset + 6, offset + 2 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // U with acute.
        released[218 - 32].resize( released[85 - 32].width(), released[85 - 32].height() + 3 );
        released[218 - 32].reset();
        fheroes2::Copy( released[85 - 32], 0, 0, released[218 - 32], 0, 3, released[85 - 32].width(), released[85 - 32].height() );
        fheroes2::DrawLine( released[218 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // U with double acute.
        released[219 - 32].resize( released[85 - 32].width(), released[85 - 32].height() + 3 );
        released[219 - 32].reset();
        fheroes2::Copy( released[85 - 32], 0, 0, released[219 - 32], 0, 3, released[85 - 32].width(), released[85 - 32].height() );
        fheroes2::DrawLine( released[219 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[219 - 32], { offset + 8, offset + 1 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );

        // U with diaeresis.
        released[220 - 32].resize( released[85 - 32].width(), released[85 - 32].height() + 3 );
        released[220 - 32].reset();
        fheroes2::Copy( released[85 - 32], 0, 0, released[220 - 32], 0, 3, released[85 - 32].width(), released[85 - 32].height() );
        fheroes2::DrawLine( released[220 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[220 - 32], { offset + 8, offset + 1 }, { offset + 9, offset + 1 }, buttonGoodReleasedColor );

        // Y with acute.
        released[221 - 32].resize( released[89 - 32].width(), released[89 - 32].height() + 3 );
        released[221 - 32].reset();
        fheroes2::Copy( released[89 - 32], 0, 0, released[221 - 32], 0, 3, released[89 - 32].width(), released[89 - 32].height() );
        fheroes2::DrawLine( released[221 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // T with cedilla. Only copied from T.
        released[222 - 32].resize( released[84 - 32].width(), released[84 - 32].height() + 3 );
        released[222 - 32].reset();
        fheroes2::Copy( released[84 - 32], 0, 0, released[222 - 32], 0, 0, released[84 - 32].width(), released[84 - 32].height() );
        fheroes2::DrawLine( released[222 - 32], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[222 - 32], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );
    }

    void generateCP1251GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), 160, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset symbols that either have diacritics or need less space to neighboring symbols.
        released[109].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[129].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[133].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        released[136].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[143].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[160].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );
        released[169].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        released[186].setPosition( buttonFontOffset.x - 1, buttonFontOffset.y );

        // K with acute, Cyrillic KJE. Needs to have upper right arm adjusted.
        released[109].resize( released[43].width(), released[43].height() + 4 );
        released[109].reset();
        fheroes2::Copy( released[43], offset + 2, offset + 1, released[109], offset + 1, offset + 4, 7, released[43].height() - offset * 2 - 2 );
        fheroes2::DrawLine( released[109], { offset + 0, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[109], { offset + 0, offset + 12 }, { offset + 2, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[109], { offset + 6, offset + 3 }, { offset + 8, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[109], { offset + 6, offset + 12 }, { offset + 8, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[109], offset + 7, offset + 11, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[109], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // ' (right single quotation mark)
        released[114].resize( 3 + offset * 2, 4 + offset * 2 );
        released[114].reset();
        fheroes2::DrawLine( released[114], { offset + 1, offset + 0 }, { offset + 1, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[114], { offset + 2, offset + 0 }, { offset + 2, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[114], offset + 0, offset + 3, buttonGoodReleasedColor );

        // y with breve.
        released[129].resize( 9 + offset * 2, 13 + offset * 2 );
        released[129].reset();
        fheroes2::DrawLine( released[129], { offset + 0, offset + 3 }, { offset + 2, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[129], { offset + 6, offset + 3 }, { offset + 8, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[129], { offset + 3, offset + 8 }, { offset + 1, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[129], { offset + 5, offset + 8 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[129], { offset + 4, offset + 8 }, { offset + 3, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[129], { offset + 0, offset + 12 }, { offset + 2, offset + 12 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[129], offset + 0, offset + 11, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[129], { offset + 3, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[129], offset + 2, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[129], offset + 6, offset + 0, buttonGoodReleasedColor );

        // J
        released[131] = released[42];

        // GHE with upturn.
        released[133].resize( 8 + offset * 2, 12 + offset * 2 );
        released[133].reset();
        fheroes2::DrawLine( released[133], { offset + 0, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[133], { offset + 7, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[133], { offset + 0, offset + 11 }, { offset + 2, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[133], { offset + 1, offset + 3 }, { offset + 1, offset + 10 }, buttonGoodReleasedColor );

        // E with two dots above.
        released[136].resize( released[37].width() - 1, released[37].height() + 3 );
        released[136].reset();
        fheroes2::Copy( released[37], offset + 1, offset + 0, released[136], offset + 0, offset + 3, released[37].width() - 1 - offset * 2,
                        released[37].height() - offset * 2 );
        fheroes2::SetPixel( released[136], offset + 3, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[136], offset + 6, offset + 1, buttonGoodReleasedColor );

        // Ukrainian IE (index 138) is made after the letter with index 189.

        // I with two dots above, Cyrillic YI
        released[143].resize( released[41].width(), released[41].height() + 3 );
        released[143].reset();
        fheroes2::Copy( released[41], 0, 0, released[143], 0, 3, released[41].width(), released[41].height() );
        fheroes2::SetPixel( released[143], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[143], offset + 4, offset + 1, buttonGoodReleasedColor );

        // I, Belarusian-Ukrainian I
        released[146] = released[41];

        // S
        released[157].resize( released[51].width() - 1, released[51].height() );
        fheroes2::Copy( released[51], 0, 0, released[157], 0, 0, 8, released[51].height() );
        fheroes2::Copy( released[51], 9, 0, released[157], 8, 0, released[51].width() - 9, released[51].height() );

        // A
        released[160].resize( 10 + offset * 2, 10 + offset * 2 );
        released[160].reset();
        fheroes2::DrawLine( released[160], { offset + 1, offset + 9 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[160], { offset + 5, offset + 0 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[160], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[160], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[160], { offset + 7, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[160], { offset + 2, offset + 6 }, { offset + 7, offset + 6 }, buttonGoodReleasedColor );

        // 6, Cyrillic BE
        released[161].resize( 8 + offset * 2, 10 + offset * 2 );
        released[161].reset();
        fheroes2::DrawLine( released[161], { offset + 0, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 0, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 7, offset + 5 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[161], offset + 7, offset + 1, buttonGoodReleasedColor );

        // B
        released[162].resize( 8 + offset * 2, 10 + offset * 2 );
        released[162].reset();
        fheroes2::DrawLine( released[162], { offset + 0, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[162], { offset + 0, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[162], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[162], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[162], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[162], { offset + 7, offset + 5 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );

        // r, Cyrillic GHE
        released[163].resize( 8 + offset * 2, 10 + offset * 2 );
        released[163].reset();
        fheroes2::DrawLine( released[163], { offset + 0, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[163], { offset + 7, offset + 1 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[163], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[163], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );

        // Cyrillic DE
        released[164].resize( 10 + offset * 2, 13 + offset * 2 );
        released[164].reset();
        fheroes2::DrawLine( released[164], { offset + 2, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[164], { offset + 8, offset + 1 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[164], { offset + 3, offset + 1 }, { offset + 3, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[164], { offset + 0, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[164], { offset + 0, offset + 10 }, { offset + 0, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[164], { offset + 9, offset + 10 }, { offset + 9, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[164], offset + 2, offset + 8, buttonGoodReleasedColor );

        // E
        released[165].resize( released[37].width() - 1, released[37].height() );
        fheroes2::Copy( released[37], 1, 0, released[165], 0, 0, released[37].width() - 1, released[37].height() );
        fheroes2::SetTransformPixel( released[165], 1, offset + 0, 1 );
        fheroes2::SetTransformPixel( released[165], 1, offset + 9, 1 );

        // X with vertical stroke through it, Cyrillic ZHE. Needs to have upper right and left arms adjusted.
        released[166].resize( released[56].width() - 1, released[56].height() );
        released[166].reset();
        fheroes2::Copy( released[56], offset + 1, offset + 0, released[166], offset + 0, offset + 0, 5, released[56].height() - offset * 2 );
        fheroes2::Copy( released[56], offset + 6, offset + 0, released[166], offset + 6, offset + 0, 5, released[56].height() - offset * 2 );
        fheroes2::DrawLine( released[166], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 4, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 4, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );

        // 3, Cyrillic ZE
        released[167].resize( 7 + offset * 2, 10 + offset * 2 );
        released[167].reset();
        fheroes2::DrawLine( released[167], { offset + 2, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 0, offset + 0 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 2, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 6, offset + 5 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 1, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[167], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[167], offset + 0, offset + 8, buttonGoodReleasedColor );

        // Mirrored N, Cyrillic I
        released[168].resize( 9 + offset * 2, 10 + offset * 2 );
        released[168].reset();
        fheroes2::DrawLine( released[168], { offset + 1, offset + 1 }, { offset + 1, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[168], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[168], { offset + 2, offset + 8 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[168], { offset + 7, offset + 0 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[168], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[168], offset + 0, offset + 9, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[168], offset + 8, offset + 0, buttonGoodReleasedColor );

        // Mirrored N with breve, Cyrillic Short I
        released[169].resize( released[168].width(), released[168].height() + 3 );
        released[169].reset();
        fheroes2::Copy( released[168], 0, 0, released[169], 0, 3, released[168].width(), released[168].height() );
        fheroes2::DrawLine( released[169], { offset + 3, offset + 1 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[169], offset + 2, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[169], offset + 6, offset + 0, buttonGoodReleasedColor );

        // K.
        released[170].resize( released[43].width() - 3, released[43].height() );
        released[170].reset();
        fheroes2::Copy( released[43], offset + 2, offset + 1, released[170], offset + 1, offset + 1, 7, released[43].height() - offset * 2 - 2 );
        fheroes2::DrawLine( released[170], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[170], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[170], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[170], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[170], offset + 7, offset + 8, buttonGoodReleasedColor );

        // /\, Cyrillic EL
        released[171].resize( 10 + offset * 2, 10 + offset * 2 );
        released[171].reset();
        fheroes2::DrawLine( released[171], { offset + 1, offset + 9 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 5, offset + 0 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 7, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );

        // M
        released[172].resize( 9 + offset * 2, 10 + offset * 2 );
        released[172].reset();
        fheroes2::DrawLine( released[172], { offset + 1, offset + 0 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[172], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[172], { offset + 2, offset + 1 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[172], { offset + 5, offset + 4 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[172], { offset + 7, offset + 0 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[172], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[172], offset + 0, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[172], offset + 8, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[172], offset + 4, offset + 4, buttonGoodReleasedColor );

        // H
        released[173].resize( 9 + offset * 2, 10 + offset * 2 );
        released[173].reset();
        fheroes2::DrawLine( released[173], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[173], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[173], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[173], { offset + 2, offset + 5 }, { offset + 7, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[173], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[173], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[173], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // O
        released[174].resize( released[47].width() - 2, released[47].height() );
        fheroes2::Copy( released[47], 0, 0, released[174], 0, 0, 7, released[47].height() );
        fheroes2::Copy( released[47], 9, 0, released[174], 7, 0, released[47].width() - 9, released[47].height() );

        // Cyrillic PE
        released[175].resize( 9 + offset * 2, 10 + offset * 2 );
        released[175].reset();
        fheroes2::DrawLine( released[175], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[175], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[175], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[175], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[175], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // P
        released[176].resize( 8 + offset * 2, 10 + offset * 2 );
        released[176].reset();
        fheroes2::DrawLine( released[176], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[176], { offset + 0, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[176], { offset + 2, offset + 5 }, { offset + 6, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[176], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[176], { offset + 7, offset + 1 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );

        // C
        released[177].resize( released[35].width() - 2, released[35].height() );
        fheroes2::Copy( released[35], 0, 0, released[177], 0, 0, 7, released[35].height() );
        fheroes2::Copy( released[35], 9, 0, released[177], 7, 0, released[35].width() - 9, released[47].height() );

        // T
        released[178].resize( released[52].width() - 2, released[52].height() );
        fheroes2::Copy( released[52], 0, 0, released[178], 0, 0, 5, released[52].height() );
        fheroes2::Copy( released[52], 6, 0, released[178], 5, 0, 3, released[52].height() );
        fheroes2::Copy( released[52], 10, 0, released[178], 8, 0, 5, released[52].height() );

        // y, Cyrillic U
        released[179].resize( 9 + offset * 2, 10 + offset * 2 );
        released[179].reset();
        fheroes2::DrawLine( released[179], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 3, offset + 5 }, { offset + 1, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 5, offset + 5 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 4, offset + 5 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[179], offset + 0, offset + 8, buttonGoodReleasedColor );

        // O with vertical bar, Cyrillic EF
        released[180].resize( 10 + offset * 2, 10 + offset * 2 );
        released[180].reset();
        fheroes2::DrawLine( released[180], { offset + 1, offset + 2 }, { offset + 7, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[180], { offset + 0, offset + 3 }, { offset + 0, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[180], { offset + 1, offset + 7 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[180], { offset + 8, offset + 3 }, { offset + 8, offset + 6 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[180], { offset + 4, offset + 1 }, { offset + 4, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[180], { offset + 3, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[180], { offset + 3, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );

        // X
        released[181].resize( released[56].width() - 3, released[56].height() );
        released[181].reset();
        fheroes2::Copy( released[56], offset + 1, offset + 0, released[181], offset + 0, offset + 0, 5, released[56].height() - offset * 2 );
        fheroes2::Copy( released[56], offset + 7, offset + 0, released[181], offset + 5, offset + 0, 4, released[56].height() - offset * 2 );

        // Cyrillic TSE
        released[182].resize( 9 + offset * 2, 12 + offset * 2 );
        released[182].reset();
        fheroes2::DrawLine( released[182], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[182], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[182], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[182], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[182], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[182], { offset + 8, offset + 10 }, { offset + 8, offset + 11 }, buttonGoodReleasedColor );

        // Cyrillic CHE
        released[183].resize( 9 + offset * 2, 10 + offset * 2 );
        released[183].reset();
        fheroes2::DrawLine( released[183], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[183], { offset + 6, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[183], { offset + 1, offset + 1 }, { offset + 1, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[183], { offset + 7, offset + 1 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[183], { offset + 2, offset + 5 }, { offset + 6, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[183], { offset + 6, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );

        // Cyrillic SHA
        released[184].resize( 11 + offset * 2, 10 + offset * 2 );
        released[184].reset();
        fheroes2::DrawLine( released[184], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[184], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[184], { offset + 9, offset + 1 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[184], { offset + 0, offset + 9 }, { offset + 10, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[184], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[184], { offset + 4, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[184], { offset + 8, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );

        // Cyrillic SHCHA
        released[185].resize( 11 + offset * 2, 12 + offset * 2 );
        released[185].reset();
        fheroes2::Copy( released[184], 0, 0, released[185], 0, 0, released[184].width(), released[184].height() );
        fheroes2::DrawLine( released[185], { offset + 10, offset + 10 }, { offset + 10, offset + 11 }, buttonGoodReleasedColor );

        // b, Cyrillic hard sign
        released[186].resize( 10 + offset * 2, 10 + offset * 2 );
        released[186].reset();
        fheroes2::DrawLine( released[186], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 2, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 3, offset + 1 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 4, offset + 4 }, { offset + 8, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 9, offset + 5 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );

        // bI, Cyrillic YERU
        released[187].resize( 10 + offset * 2, 10 + offset * 2 );
        released[187].reset();
        fheroes2::DrawLine( released[187], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 2, offset + 4 }, { offset + 4, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 5, offset + 5 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::Copy( released[41], offset + 1, 0, released[187], offset + 7, 0, released[41].width() - 2 - offset * 2, released[41].height() );

        // b, Cyrillic soft sign
        released[188].resize( 8 + offset * 2, 10 + offset * 2 );
        released[188].reset();
        fheroes2::DrawLine( released[188], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 0, offset + 9 }, { offset + 6, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 1, offset + 1 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 7, offset + 5 }, { offset + 7, offset + 8 }, buttonGoodReleasedColor );

        // Flipped C with line inside, Cyrillic E
        released[189].resize( 8 + offset * 2, 10 + offset * 2 );
        released[189].reset();
        fheroes2::DrawLine( released[189], { offset + 2, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[189], { offset + 7, offset + 2 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[189], { offset + 2, offset + 9 }, { offset + 5, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[189], { offset + 0, offset + 0 }, { offset + 0, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[189], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[189], offset + 0, offset + 7, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[189], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[189], offset + 1, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[189], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[189], offset + 6, offset + 8, buttonGoodReleasedColor );

        // Ukrainian IE. Make it by mirroring horizontally the previous letter.
        released[138].resize( 8 + offset * 2, 10 + offset * 2 );
        released[138].reset();
        fheroes2::Blit( released[189], released[138], true );

        // IO, Cyrillic YU
        released[190].resize( 11 + offset * 2, 10 + offset * 2 );
        released[190].reset();
        fheroes2::Copy( released[41], offset + 1, 0, released[190], offset + 0, 0, released[41].width() - 2 - offset * 2, released[41].height() );
        fheroes2::Copy( released[47], offset, 0, released[190], offset + 4, 0, 3, released[47].height() );
        fheroes2::Copy( released[47], offset + 6, 0, released[190], offset + 7, 0, 4, released[47].height() );
        fheroes2::DrawLine( released[190], { offset + 2, offset + 4 }, { offset + 3, offset + 4 }, buttonGoodReleasedColor );

        // Mirrored R, Cyrillic YA
        released[191].resize( released[176].width(), released[176].height() );
        fheroes2::Flip( released[176], 0, 0, released[191], 0, 0, released[176].width(), released[176].height(), true, false );
        fheroes2::DrawLine( released[191], { offset + 0, offset + 9 }, { offset + 2, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[191], { offset + 3, offset + 6 }, { offset + 1, offset + 8 }, buttonGoodReleasedColor );
    }

    void generateCP1252GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        const fheroes2::Sprite firstSprite{ released[0] };
        released.insert( released.end(), 160, firstSprite );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        for ( const int & charCode : { 192, 193, 194, 195, 200, 201, 202, 205, 209, 211, 212, 213, 218 } ) {
            released[charCode - 32].setPosition( buttonFontOffset.x, buttonFontOffset.y - 3 );
        }

        for ( const int & charCode : { 196, 197, 214, 220 } ) {
            released[charCode - 32].setPosition( buttonFontOffset.x, buttonFontOffset.y - 2 );
        }

        // Offset A-related letters to have less space to neighboring letters. Keep the y-offset change from earlier.
        for ( const int & charCode : { 65, 192, 193, 194, 195, 196, 197, 198 } ) {
            released[charCode - 32].setPosition( buttonFontOffset.x - 2, released[charCode - 32].y() );
        }

        // A with grave.
        released[192 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[192 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[192 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[192 - 32], { offset + 6, offset + 0 }, { offset + 7, offset + 1 }, buttonGoodReleasedColor );

        // A with acute.
        released[193 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[193 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[193 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[193 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // A with circumflex
        released[194 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[194 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[194 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[194 - 32], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[194 - 32], offset + 7, offset + 1, buttonGoodReleasedColor );

        // A with tilde
        released[195 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 3 );
        released[195 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[195 - 32], 0, 3, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::DrawLine( released[195 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[195 - 32], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[195 - 32], offset + 7, offset + 1, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[195 - 32], { offset + 8, offset + 1 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );

        // A with diaeresis
        released[196 - 32].resize( released[65 - 32].width(), released[65 - 32].height() + 2 );
        released[196 - 32].reset();
        fheroes2::Copy( released[65 - 32], 0, 0, released[196 - 32], 0, 2, released[65 - 32].width(), released[65 - 32].height() );
        fheroes2::SetPixel( released[196 - 32], offset + 5, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[196 - 32], offset + 7, offset + 0, buttonGoodReleasedColor );

        // A with circle on top
        released[197 - 32].resize( 13 + offset * 2, 12 + offset * 2 );
        released[197 - 32].reset();
        fheroes2::DrawLine( released[197 - 32], { offset + 0, offset + 11 }, { offset + 4, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 8, offset + 11 }, { offset + 12, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 5, offset + 7 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 2, offset + 10 }, { offset + 4, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 7, offset + 3 }, { offset + 10, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 8, offset + 1 }, { offset + 8, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[197 - 32], { offset + 6, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197 - 32], offset + 4, offset + 6, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197 - 32], offset + 5, offset + 5, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197 - 32], offset + 5, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[197 - 32], offset + 6, offset + 3, buttonGoodReleasedColor );

        // A attached to E.
        released[198 - 32].resize( 15 + offset * 2, 10 + offset * 2 );
        released[198 - 32].reset();
        fheroes2::DrawLine( released[198 - 32], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 5, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 2, offset + 8 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 7, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 7, offset + 0 }, { offset + 14, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 13, offset + 9 }, { offset + 14, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[198 - 32], { offset + 9, offset + 4 }, { offset + 12, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 4, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 5, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 5, offset + 2, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 14, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 14, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 12, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[198 - 32], offset + 12, offset + 5, buttonGoodReleasedColor );

        // C with cedilla.
        released[199 - 32].resize( released[67 - 32].width(), released[67 - 32].height() + 3 );
        released[199 - 32].reset();
        fheroes2::Copy( released[67 - 32], 0, 0, released[199 - 32], 0, 0, released[67 - 32].width(), released[67 - 32].height() );
        fheroes2::DrawLine( released[199 - 32], { offset + 5, offset + 10 }, { offset + 6, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[199 - 32], { offset + 4, offset + 12 }, { offset + 5, offset + 12 }, buttonGoodReleasedColor );

        // E with grave.
        released[200 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[200 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[200 - 32], 0, 3, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::DrawLine( released[200 - 32], { offset + 4, offset + 0 }, { offset + 5, offset + 1 }, buttonGoodReleasedColor );

        // E with acute.
        released[201 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[201 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[201 - 32], 0, 3, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::DrawLine( released[201 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // E with circumflex.
        released[202 - 32].resize( released[69 - 32].width(), released[69 - 32].height() + 3 );
        released[202 - 32].reset();
        fheroes2::Copy( released[69 - 32], 0, 0, released[202 - 32], 0, 3, released[69 - 32].width(), released[69 - 32].height() );
        fheroes2::Copy( released[194 - 32], offset + 5, offset, released[202 - 32], offset + 3, offset, 3, 2 );

        // I with acute.
        released[205 - 32].resize( released[73 - 32].width(), released[73 - 32].height() + 3 );
        released[205 - 32].reset();
        fheroes2::Copy( released[73 - 32], 0, 0, released[205 - 32], 0, 3, released[73 - 32].width(), released[73 - 32].height() );
        fheroes2::DrawLine( released[205 - 32], { offset + 2, offset + 1 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );

        // N with tilde.
        released[209 - 32].resize( released[78 - 32].width(), released[78 - 32].height() + 3 );
        released[209 - 32].reset();
        fheroes2::Copy( released[78 - 32], 0, 0, released[209 - 32], 0, 3, released[78 - 32].width(), released[78 - 32].height() );
        fheroes2::Copy( released[195 - 32], offset + 4, offset, released[209 - 32], offset + 4, offset, 6, 2 );

        // O with acute.
        released[211 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[211 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[211 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::DrawLine( released[211 - 32], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // O with circumflex.
        released[212 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[212 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[212 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::DrawLine( released[212 - 32], { offset + 3, offset + 1 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[212 - 32], { offset + 5, offset + 0 }, { offset + 6, offset + 1 }, buttonGoodReleasedColor );

        // O with tilde.
        released[213 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 3 );
        released[213 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[213 - 32], 0, 3, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::Copy( released[195 - 32], offset + 4, offset, released[213 - 32], offset + 2, offset, 6, 2 );

        // O with diaeresis.
        released[214 - 32].resize( released[79 - 32].width(), released[79 - 32].height() + 2 );
        released[214 - 32].reset();
        fheroes2::Copy( released[79 - 32], 0, 0, released[214 - 32], 0, 2, released[79 - 32].width(), released[79 - 32].height() );
        fheroes2::SetPixel( released[214 - 32], offset + 3, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[214 - 32], offset + 6, offset + 0, buttonGoodReleasedColor );

        // U with acute.
        released[218 - 32].resize( released[85 - 32].width(), released[85 - 32].height() + 3 );
        released[218 - 32].reset();
        fheroes2::Copy( released[85 - 32], 0, 0, released[218 - 32], 0, 3, released[85 - 32].width(), released[85 - 32].height() );
        fheroes2::DrawLine( released[218 - 32], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // U with diaeresis.
        released[220 - 32].resize( released[85 - 32].width(), released[85 - 32].height() + 2 );
        released[220 - 32].reset();
        fheroes2::Copy( released[85 - 32], 0, 0, released[220 - 32], 0, 2, released[85 - 32].width(), released[85 - 32].height() );
        fheroes2::SetPixel( released[220 - 32], offset + 4, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[220 - 32], offset + 8, offset + 0, buttonGoodReleasedColor );
    }
}

namespace fheroes2
{
    void generateAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite )
    {
        switch ( language ) {
        case SupportedLanguage::Czech:
        case SupportedLanguage::Hungarian:
        case SupportedLanguage::Polish:
        case SupportedLanguage::Slovak:
            generateCP1250Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::Belarusian:
        case SupportedLanguage::Bulgarian:
        case SupportedLanguage::Russian:
        case SupportedLanguage::Ukrainian:
            generateCP1251Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::Danish:
        case SupportedLanguage::Dutch:
        case SupportedLanguage::German:
        case SupportedLanguage::Italian:
        case SupportedLanguage::Norwegian:
        case SupportedLanguage::Portuguese:
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
            generateCP1252Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::French:
            generateCP1252Alphabet( icnVsSprite );
            // This serves to make the font compatible with the original French custom encoding.
            generateFrenchAlphabet( icnVsSprite );
            break;
        case SupportedLanguage::Turkish:
            generateCP1254Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::Vietnamese:
            generateCP1258Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::Romanian:
            generateISO8859_16Alphabet( icnVsSprite );
            break;
        default:
            // Add new language generation code!
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

    bool isAlphabetSupported( const SupportedLanguage language )
    {
        switch ( language ) {
        case SupportedLanguage::Polish:
        case SupportedLanguage::German:
        case SupportedLanguage::French:
        case SupportedLanguage::Italian:
        case SupportedLanguage::Norwegian:
        case SupportedLanguage::Russian:
        case SupportedLanguage::Belarusian:
        case SupportedLanguage::Bulgarian:
        case SupportedLanguage::Portuguese:
        case SupportedLanguage::Romanian:
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
        case SupportedLanguage::Turkish:
        case SupportedLanguage::Ukrainian:
        case SupportedLanguage::Dutch:
        case SupportedLanguage::Hungarian:
        case SupportedLanguage::Czech:
        case SupportedLanguage::Danish:
        case SupportedLanguage::Slovak:
        case SupportedLanguage::Vietnamese:
            return true;
        default:
            break;
        }

        return false;
    }

    void generateBaseButtonFont( std::vector<Sprite> & goodReleased, std::vector<Sprite> & goodPressed, std::vector<Sprite> & evilReleased,
                                 std::vector<Sprite> & evilPressed )
    {
        generateGoodButtonFontBaseShape( goodReleased );

        updateButtonFont( goodReleased, goodPressed, evilReleased, evilPressed );
    }

    void generateButtonAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite )
    {
        generateGoodButtonFontBaseShape( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );

        switch ( language ) {
        case SupportedLanguage::English:
            generateBaseButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED],
                                    icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] );
            return;
        case SupportedLanguage::Czech:
        case SupportedLanguage::Hungarian:
        case SupportedLanguage::Polish:
        case SupportedLanguage::Slovak:
            generateCP1250GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Belarusian:
        case SupportedLanguage::Bulgarian:
        case SupportedLanguage::Russian:
        case SupportedLanguage::Ukrainian:
            generateCP1251GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Danish:
        case SupportedLanguage::Dutch:
        case SupportedLanguage::French:
        case SupportedLanguage::German:
        case SupportedLanguage::Italian:
        case SupportedLanguage::Norwegian:
        case SupportedLanguage::Portuguese:
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
            generateCP1252GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Turkish:
            // generateGoodCP1254ButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Vietnamese:
            // generateGoodCP1258ButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Romanian:
            // generateGoodISO8859_16ButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        default:
            // Add new language generation code!
            assert( 0 );
            break;
        }

        updateButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED],
                          icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] );
    }

    void modifyBaseNormalFont( std::vector<fheroes2::Sprite> & icnVsSprite )
    {
        if ( icnVsSprite.size() < 96 ) {
            return;
        }

        // Remove white line from % symbol
        fheroes2::FillTransform( icnVsSprite[5], 5, 0, 5, 1, 1 );
        fheroes2::FillTransform( icnVsSprite[5], 6, 2, 2, 1, 1 );
        updateNormalFontLetterShadow( icnVsSprite[5] );

        // Move "-" further down
        icnVsSprite[13].setPosition( icnVsSprite[13].x(), icnVsSprite[13].y() + 1 );
        updateNormalFontLetterShadow( icnVsSprite[13] );

        // Add the '\' character.
        icnVsSprite[60].resize( 8, 14 );
        icnVsSprite[60].reset();
        fheroes2::Blit( icnVsSprite[15], 0, 0, icnVsSprite[60], 1, 0, 7, 12, true );
        icnVsSprite[60].setPosition( icnVsSprite[15].x(), icnVsSprite[15].y() );
        updateNormalFontLetterShadow( icnVsSprite[60] );

        // Proper lowercase k.
        fheroes2::FillTransform( icnVsSprite[75], 4, 1, 5, 8, 1 );
        fheroes2::Copy( icnVsSprite[43], 6, 5, icnVsSprite[75], 4, 7, 3, 1 );
        fheroes2::Copy( icnVsSprite[43], 6, 4, icnVsSprite[75], 4, 6, 4, 1 );
        fheroes2::Copy( icnVsSprite[43], 7, 4, icnVsSprite[75], 6, 5, 3, 1 );
        fheroes2::Copy( icnVsSprite[43], 7, 4, icnVsSprite[75], 7, 4, 2, 1 );
        fheroes2::Copy( icnVsSprite[43], 6, 6, icnVsSprite[75], 4, 8, 4, 1 );
        icnVsSprite[75].setPosition( icnVsSprite[75].x(), icnVsSprite[75].y() );
        updateNormalFontLetterShadow( icnVsSprite[75] );

        // System call 'DELETE' (0x7F) is never used as a text character in phrases.
        // To make the blinking text cursor we have to make a transparent character with the width of the cursor '_'.
        icnVsSprite[127 - 32].resize( icnVsSprite[95 - 32].width(), 1 );
        icnVsSprite[127 - 32].reset();
    }

    void modifyBaseSmallFont( std::vector<fheroes2::Sprite> & icnVsSprite )
    {
        if ( icnVsSprite.size() < 96 ) {
            return;
        }

        // Remove white line from % symbol
        fheroes2::FillTransform( icnVsSprite[5], 3, 0, 4, 1, 1 );
        fheroes2::FillTransform( icnVsSprite[5], 4, 1, 2, 1, 1 );
        updateNormalFontLetterShadow( icnVsSprite[5] );

        // Add the '\' character.
        icnVsSprite[60].resize( 5, 9 );
        icnVsSprite[60].reset();
        fheroes2::Copy( icnVsSprite[15], 4, 0, icnVsSprite[60], 1, 0, 1, 2 );
        fheroes2::Copy( icnVsSprite[15], 4, 0, icnVsSprite[60], 2, 2, 1, 2 );
        fheroes2::Copy( icnVsSprite[15], 4, 0, icnVsSprite[60], 3, 4, 1, 2 );
        fheroes2::Copy( icnVsSprite[15], 4, 0, icnVsSprite[60], 4, 6, 1, 2 );
        icnVsSprite[60].setPosition( icnVsSprite[15].x(), icnVsSprite[15].y() );
        updateSmallFontLetterShadow( icnVsSprite[60] );

        // Proper lowercase k.
        icnVsSprite[75].resize( 6, 8 );
        icnVsSprite[75].reset();
        fheroes2::Copy( icnVsSprite[76], 1, 0, icnVsSprite[75], 1, 0, 2, 7 );
        fheroes2::Copy( icnVsSprite[76], 1, 0, icnVsSprite[75], 1, 6, 1, 1 );
        fheroes2::Copy( icnVsSprite[56], 6, 0, icnVsSprite[75], 3, 2, 3, 3 );
        fheroes2::Copy( icnVsSprite[65], 2, icnVsSprite[65].height() - 2, icnVsSprite[75], 5, 6, 2, 1 );
        fheroes2::Copy( icnVsSprite[65], 2, 0, icnVsSprite[75], 4, 5, 1, 1 );
        icnVsSprite[75].setPosition( icnVsSprite[75].x(), icnVsSprite[75].y() );
        updateSmallFontLetterShadow( icnVsSprite[75] );
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

        const int32_t centerY = std::max( 1, ( height / 2 ) - height % 2 );
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
}
