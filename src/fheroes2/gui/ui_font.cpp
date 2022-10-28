/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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
#include "icn.h"

#include <cassert>

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
        fheroes2::updateShadow( letter, { -1, 2 }, 2 );
    }

    void updateSmallFontLetterShadow( fheroes2::Image & letter )
    {
        fheroes2::updateShadow( letter, { -1, 1 }, 2 );
    }

    fheroes2::Sprite addContour( fheroes2::Sprite & input, const fheroes2::Point & contourOffset, const uint8_t colorId )
    {
        if ( input.empty() || contourOffset.x > 0 || contourOffset.y < 0 || ( -contourOffset.x >= input.width() ) || ( contourOffset.y >= input.height() ) )
            return input;

        fheroes2::Sprite output = input;

        const int32_t width = output.width() + contourOffset.x;
        const int32_t height = output.height() - contourOffset.y;

        const int32_t imageWidth = output.width();

        uint8_t * imageOutY = output.image() + imageWidth * contourOffset.y;
        const uint8_t * transformInY = input.transform() - contourOffset.x;
        uint8_t * transformOutY = output.transform() + imageWidth * contourOffset.y;
        const uint8_t * transformOutYEnd = transformOutY + imageWidth * height;

        for ( ; transformOutY != transformOutYEnd; transformInY += imageWidth, transformOutY += imageWidth, imageOutY += imageWidth ) {
            uint8_t * imageOutX = imageOutY;
            const uint8_t * transformInX = transformInY;
            uint8_t * transformOutX = transformOutY;
            const uint8_t * transformOutXEnd = transformOutX + width;

            for ( ; transformOutX != transformOutXEnd; ++transformInX, ++transformOutX, ++imageOutX ) {
                if ( *transformInX == 0 && *transformOutX == 1 ) {
                    *imageOutX = colorId;
                    *transformOutX = 0;
                }
            }
        }

        return output;
    }

    void applyGoodButtonReleasedLetterEffects( fheroes2::Sprite & letter )
    {
        updateShadow( letter, { 1, -1 }, 2 );
        updateShadow( letter, { 2, -2 }, 4 );
        letter = addContour( letter, { -1, 1 }, buttonContourColor );
        updateShadow( letter, { -1, 1 }, 7 );
    }

    void applyGoodButtonPressedLetterEffects( fheroes2::Sprite & letter )
    {
        ReplaceColorId( letter, buttonGoodReleasedColor, buttonGoodPressedColor );

        fheroes2::updateShadow( letter, { 1, -1 }, 2 );
        fheroes2::updateShadow( letter, { -1, 1 }, 7 );
        fheroes2::updateShadow( letter, { -2, 2 }, 8 );
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
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, icnVsSprite[icnId][0] );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // Uppercase S with caron
            font[138 - 32] = font[83 - 32];

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
            font[141 - 32] = font[84 - 32];
            // Uppercase Z with caron
            font[142 - 32] = font[90 - 32];

            // Uppercase Z with acute
            font[143 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[143 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[143 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[143 - 32], 4, 0, 3, 2 );
            font[143 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[143 - 32] );

            // Lowercase s with caron
            font[154 - 32] = font[115 - 32];

            // Lowercase s with acute
            font[156 - 32].resize( font[115 - 32].width(), font[115 - 32].height() + 3 );
            font[156 - 32].reset();
            fheroes2::Copy( font[115 - 32], 0, 0, font[156 - 32], 0, 3, font[115 - 32].width(), font[115 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[156 - 32], 4, 0, 3, 2 );
            font[156 - 32].setPosition( font[115 - 32].x(), font[115 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[156 - 32] );

            // Lowercase t with caron
            font[157 - 32] = font[116 - 32];
            // Lowercase z with caron
            font[158 - 32] = font[122 - 32];

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

            // Lowercase z with dot above
            font[191 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 3 );
            font[191 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[191 - 32], 0, 3, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[175 - 32], 5, 0, font[191 - 32], 4, 0, 2, 2 );
            font[191 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[191 - 32] );

            // Uppercase A with acute
            font[193 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[193 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[193 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[193 - 32], 7, 0, 3, 2 );
            font[193 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[193 - 32] );

            // Uppercase C with acute
            font[198 - 32].resize( font[67 - 32].width(), font[67 - 32].height() + 3 );
            font[198 - 32].reset();
            fheroes2::Copy( font[67 - 32], 0, 0, font[198 - 32], 0, 3, font[67 - 32].width(), font[67 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[198 - 32], 8, 0, 3, 2 );
            font[198 - 32].setPosition( font[67 - 32].x(), font[67 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[198 - 32] );

            // Uppercase C with caron
            font[200 - 32] = font[67 - 32];

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
            font[204 - 32] = font[69 - 32];

            // Uppercase I with acute
            font[205 - 32].resize( font[73 - 32].width(), font[73 - 32].height() + 3 );
            font[205 - 32].reset();
            fheroes2::Copy( font[73 - 32], 0, 0, font[205 - 32], 0, 3, font[73 - 32].width(), font[73 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[205 - 32], 4, 0, 3, 2 );
            font[205 - 32].setPosition( font[73 - 32].x(), font[73 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[205 - 32] );

            // Uppercase D with caron
            font[207 - 32] = font[68 - 32];

            // Uppercase N with acute
            font[209 - 32].resize( font[78 - 32].width(), font[78 - 32].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[78 - 32], 0, 0, font[209 - 32], 0, 3, font[78 - 32].width(), font[78 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[209 - 32], 8, 0, 3, 2 );
            font[209 - 32].setPosition( font[78 - 32].x(), font[78 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[209 - 32] );

            // Uppercase N with caron
            font[210 - 32] = font[78 - 32];

            // Uppercase O with acute
            font[211 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[211 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[211 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[211 - 32], 8, 0, 3, 2 );
            font[211 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[211 - 32] );

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
            font[216 - 32] = font[82 - 32];
            // Uppercase U with ring above
            font[217 - 32] = font[85 - 32];

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

            // Lowercase a with acute
            font[225 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[225 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[225 - 32], 3, 0, 3, 2 );
            font[225 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[225 - 32] );

            // Lowercase c with acute
            font[230 - 32].resize( font[99 - 32].width(), font[99 - 32].height() + 3 );
            font[230 - 32].reset();
            fheroes2::Copy( font[99 - 32], 0, 0, font[230 - 32], 0, 3, font[99 - 32].width(), font[99 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[230 - 32], 4, 0, 3, 2 );
            font[230 - 32].setPosition( font[99 - 32].x(), font[99 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[230 - 32] );

            // Lowercase c with caron
            font[232 - 32] = font[99 - 32];

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
            font[236 - 32] = font[101 - 32];

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
            font[242 - 32] = font[110 - 32];

            // Lowercase o with acute
            font[243 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[243 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[140 - 32], 4, 0, font[243 - 32], 4, 0, 3, 2 );
            font[243 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateNormalFontLetterShadow( font[243 - 32] );

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
            font[248 - 32] = font[114 - 32];
            // Lowercase u with ring above
            font[249 - 32] = font[117 - 32];

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

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            fheroes2::FillTransform( font[75], 4, 1, 5, 8, 1 );
            fheroes2::Copy( font[43], 6, 5, font[75], 4, 7, 3, 1 );
            fheroes2::Copy( font[43], 6, 4, font[75], 4, 6, 4, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 6, 5, 3, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 7, 4, 2, 1 );
            fheroes2::Copy( font[43], 6, 6, font[75], 4, 8, 4, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateNormalFontLetterShadow( font[75] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // Uppercase S with caron
            font[138 - 32] = font[83 - 32];

            // Uppercase S with acute
            font[140 - 32].resize( font[83 - 32].width(), font[83 - 32].height() + 3 );
            font[140 - 32].reset();
            fheroes2::Copy( font[83 - 32], 0, 0, font[140 - 32], 0, 3, font[83 - 32].width(), font[83 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[140 - 32], 4, 0, 2, 2 );
            font[140 - 32].setPosition( font[83 - 32].x(), font[83 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[140 - 32] );

            // Uppercase T with caron
            font[138 - 32] = font[84 - 32];
            // Uppercase Z with caron
            font[142 - 32] = font[90 - 32];

            // Uppercase Z with acute
            font[143 - 32].resize( font[90 - 32].width(), font[90 - 32].height() + 3 );
            font[143 - 32].reset();
            fheroes2::Copy( font[90 - 32], 0, 0, font[143 - 32], 0, 3, font[90 - 32].width(), font[90 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[143 - 32], 4, 0, 2, 2 );
            font[143 - 32].setPosition( font[90 - 32].x(), font[90 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[143 - 32] );

            // Lowercase s with caron
            font[154 - 32] = font[115 - 32];

            // Lowercase s with acute
            font[156 - 32].resize( font[115 - 32].width(), font[115 - 32].height() + 3 );
            font[156 - 32].reset();
            fheroes2::Copy( font[115 - 32], 0, 0, font[156 - 32], 0, 3, font[115 - 32].width(), font[115 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[156 - 32], 3, 0, 2, 2 );
            font[156 - 32].setPosition( font[115 - 32].x(), font[115 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[156 - 32] );

            // Lowercase t with caron
            font[157 - 32] = font[116 - 32];
            // Lowercase z with caron
            font[158 - 32] = font[122 - 32];

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

            // Lowercase z with dot above
            font[191 - 32].resize( font[122 - 32].width(), font[122 - 32].height() + 2 );
            font[191 - 32].reset();
            fheroes2::Copy( font[122 - 32], 0, 0, font[191 - 32], 0, 2, font[122 - 32].width(), font[122 - 32].height() );
            fheroes2::Copy( font[90 - 32], 2, 0, font[191 - 32], 3, 0, 2, 1 );
            font[191 - 32].setPosition( font[122 - 32].x(), font[122 - 32].y() - 2 );
            updateSmallFontLetterShadow( font[191 - 32] );

            // Uppercase A with acute
            font[193 - 32].resize( font[65 - 32].width(), font[65 - 32].height() + 3 );
            font[193 - 32].reset();
            fheroes2::Copy( font[65 - 32], 0, 0, font[193 - 32], 0, 3, font[65 - 32].width(), font[65 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[193 - 32], 5, 0, 2, 2 );
            font[193 - 32].setPosition( font[65 - 32].x(), font[65 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[193 - 32] );

            // Uppercase C with acute
            font[198 - 32].resize( font[67 - 32].width(), font[67 - 32].height() + 3 );
            font[198 - 32].reset();
            fheroes2::Copy( font[67 - 32], 0, 0, font[198 - 32], 0, 3, font[67 - 32].width(), font[67 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[198 - 32], 4, 0, 2, 2 );
            font[198 - 32].setPosition( font[67 - 32].x(), font[67 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[198 - 32] );

            // Uppercase C with caron
            font[200 - 32] = font[67 - 32];

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
            font[204 - 32] = font[69 - 32];

            // Uppercase I with acute
            font[205 - 32].resize( font[73 - 32].width(), font[73 - 32].height() + 3 );
            font[205 - 32].reset();
            fheroes2::Copy( font[73 - 32], 0, 0, font[205 - 32], 0, 3, font[73 - 32].width(), font[73 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[205 - 32], 2, 0, 2, 2 );
            font[205 - 32].setPosition( font[73 - 32].x(), font[73 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[205 - 32] );

            // Uppercase D with caron
            font[207 - 32] = font[68 - 32];

            // Uppercase N with acute
            font[209 - 32].resize( font[78 - 32].width(), font[78 - 32].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[78 - 32], 0, 0, font[209 - 32], 0, 3, font[78 - 32].width(), font[78 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[209 - 32], 5, 0, 2, 2 );
            font[209 - 32].setPosition( font[78 - 32].x(), font[78 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[209 - 32] );

            // Uppercase N with caron
            font[210 - 32] = font[78 - 32];

            // Uppercase O with acute
            font[211 - 32].resize( font[79 - 32].width(), font[79 - 32].height() + 3 );
            font[211 - 32].reset();
            fheroes2::Copy( font[79 - 32], 0, 0, font[211 - 32], 0, 3, font[79 - 32].width(), font[79 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[211 - 32], 4, 0, 2, 2 );
            font[211 - 32].setPosition( font[79 - 32].x(), font[79 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[211 - 32] );

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
            font[216 - 32] = font[82 - 32];
            // Uppercase U with ring above
            font[217 - 32] = font[85 - 32];

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

            // Lowercase a with acute
            font[225 - 32].resize( font[97 - 32].width(), font[97 - 32].height() + 3 );
            font[225 - 32].reset();
            fheroes2::Copy( font[97 - 32], 0, 0, font[225 - 32], 0, 3, font[97 - 32].width(), font[97 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[225 - 32], 3, 0, 2, 2 );
            font[225 - 32].setPosition( font[97 - 32].x(), font[97 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[225 - 32] );

            // Lowercase c with acute
            font[230 - 32].resize( font[99 - 32].width(), font[99 - 32].height() + 3 );
            font[230 - 32].reset();
            fheroes2::Copy( font[99 - 32], 0, 0, font[230 - 32], 0, 3, font[99 - 32].width(), font[99 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[230 - 32], 3, 0, 2, 2 );
            font[230 - 32].setPosition( font[99 - 32].x(), font[99 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[230 - 32] );

            // Lowercase c with caron
            font[232 - 32] = font[99 - 32];

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
            font[236 - 32] = font[101 - 32];

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
            font[242 - 32] = font[110 - 32];

            // Lowercase o with acute
            font[243 - 32].resize( font[111 - 32].width(), font[111 - 32].height() + 3 );
            font[243 - 32].reset();
            fheroes2::Copy( font[111 - 32], 0, 0, font[243 - 32], 0, 3, font[111 - 32].width(), font[111 - 32].height() );
            fheroes2::Copy( font[122 - 32], 2, 2, font[243 - 32], 3, 0, 2, 2 );
            font[243 - 32].setPosition( font[111 - 32].x(), font[111 - 32].y() - 3 );
            updateSmallFontLetterShadow( font[243 - 32] );

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
            font[248 - 32] = font[114 - 32];
            // Lowercase u with ring above
            font[249 - 32] = font[117 - 32];

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

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            font[75].resize( 6, 8 );
            font[75].reset();
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 0, 2, 7 );
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 6, 1, 1 );
            fheroes2::Copy( font[56], 6, 0, font[75], 3, 2, 3, 3 );
            fheroes2::Copy( font[65], 2, font[65].height() - 2, font[75], 5, 6, 2, 1 );
            fheroes2::Copy( font[65], 2, 0, font[75], 4, 5, 1, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateSmallFontLetterShadow( font[75] );
        }
    }

    void generateFrenchAlphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            font[3].resize( font[79].width(), font[79].height() + 3 );
            font[3].reset();
            fheroes2::Copy( font[79], 0, 0, font[3], 0, 3, font[79].width(), font[79].height() );
            // generate ^ on the top.
            fheroes2::Copy( font[3], 2, 3, font[3], 3, 0, 1, 1 );
            fheroes2::Copy( font[3], 4, 3, font[3], 4, 0, 1, 1 );
            fheroes2::Copy( font[3], 2, 3, font[3], 5, 0, 1, 1 );
            fheroes2::Copy( font[3], 2, 3, font[3], 2, 1, 1, 1 );
            fheroes2::Copy( font[3], 4, 3, font[3], 3, 1, 1, 1 );
            fheroes2::Copy( font[3], 4, 3, font[3], 5, 1, 1, 1 );
            fheroes2::Copy( font[3], 2, 3, font[3], 6, 1, 1, 1 );
            font[3].setPosition( font[79].x(), font[79].y() - 3 );
            updateNormalFontLetterShadow( font[3] );

            font[4].resize( font[85].width(), font[85].height() + 3 );
            font[4].reset();
            fheroes2::Copy( font[85], 0, 0, font[4], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[3], 2, 0, font[4], 3, 0, 5, 2 );
            font[4].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[4] );

            font[6].resize( font[85].width(), font[85].height() + 3 );
            font[6].reset();
            // generate -_ on the top.
            fheroes2::Copy( font[85], 0, 0, font[6], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[6], 2, 3, font[6], 4, 0, 1, 1 );
            fheroes2::Copy( font[6], 2, 4, font[6], 5, 0, 1, 1 );
            fheroes2::Copy( font[6], 2, 3, font[6], 5, 1, 1, 1 );
            fheroes2::Copy( font[6], 2, 4, font[6], 6, 1, 1, 1 );
            fheroes2::Copy( font[6], 3, 3, font[6], 7, 1, 1, 1 );
            font[6].setPosition( font[85].x(), font[85].y() - 3 );
            updateNormalFontLetterShadow( font[6] );

            font[10].resize( font[65].width(), font[65].height() + 3 );
            font[10].reset();
            fheroes2::Copy( font[65], 0, 0, font[10], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[3], 2, 0, font[10], 2, 0, 5, 2 );
            font[10].setPosition( font[65].x(), font[65].y() - 3 );
            updateNormalFontLetterShadow( font[10] );

            font[28] = font[73];
            // Just to be safe and not to write something out of buffer.
            if ( font[28].width() > 2 && font[28].height() > 1 ) {
                font[28].image()[2] = 0;
                font[28].transform()[2] = 1;
                font[28].image()[2 + font[28].width()] = 0;
                font[28].transform()[2 + font[28].width()] = 1;
            }
            fheroes2::Copy( font[28], 3, 0, font[28], 1, 0, 1, 2 );
            updateNormalFontLetterShadow( font[28] );

            font[30] = font[73];
            // Just to be safe and not to write something out of buffer.
            if ( font[30].width() > 4 && font[30].height() > 1 ) {
                font[30].image()[1] = 0;
                font[30].transform()[1] = 1;
                font[30].image()[3] = 0;
                font[30].transform()[3] = 1;
                font[30].image()[2 + font[30].width()] = 0;
                font[30].transform()[2 + font[30].width()] = 1;
            }
            fheroes2::Copy( font[30], 2, 0, font[30], 1, 1, 1, 1 );
            fheroes2::Copy( font[30], 2, 0, font[30], 3, 1, 1, 1 );
            updateNormalFontLetterShadow( font[30] );

            font[32].resize( font[65].width(), font[65].height() + 3 );
            font[32].reset();
            fheroes2::Copy( font[65], 0, 0, font[32], 0, 3, font[65].width(), font[65].height() );
            font[32].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[6], 4, 0, font[32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[32] );

            font[62].resize( font[67].width(), font[67].height() + 2 );
            font[62].reset();
            fheroes2::Copy( font[67], 0, 0, font[62], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[67], 2, 1, font[62], 4, 7, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[62], 5, 7, 1, 1 );
            fheroes2::Copy( font[67], 2, 6, font[62], 6, 7, 1, 1 );
            fheroes2::Copy( font[67], 2, 1, font[62], 5, 8, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[62], 6, 8, 1, 1 );
            fheroes2::Copy( font[67], 2, 1, font[62], 4, 9, 1, 1 );
            fheroes2::Copy( font[67], 5, 6, font[62], 5, 9, 1, 1 );
            fheroes2::Copy( font[67], 3, 0, font[62], 6, 9, 1, 1 );
            font[62].setPosition( font[67].x(), font[67].y() );
            updateNormalFontLetterShadow( font[62] );

            font[64].resize( font[69].width(), font[69].height() + 3 );
            font[64].reset();
            fheroes2::Copy( font[69], 0, 0, font[64], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[64], 4, 3, font[64], 4, 0, 1, 1 );
            fheroes2::Copy( font[64], 4, 3, font[64], 5, 1, 1, 1 );
            fheroes2::Copy( font[64], 8, 6, font[64], 5, 0, 1, 1 );
            fheroes2::Copy( font[64], 8, 6, font[64], 6, 1, 1, 1 );
            fheroes2::Copy( font[64], 4, 8, font[64], 6, 2, 1, 1 );
            font[64].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[64] );

            font[91] = font[28];

            font[92].resize( font[69].width(), font[69].height() + 3 );
            font[92].reset();
            fheroes2::Copy( font[69], 0, 0, font[92], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[3], 2, 0, font[92], 3, 0, 5, 2 );
            font[92].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[92] );

            font[93] = font[30];

            font[94].resize( font[69].width(), font[69].height() + 3 );
            font[94].reset();
            fheroes2::Copy( font[69], 0, 0, font[94], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[94], 4, 8, font[94], 3, 1, 1, 1 );
            fheroes2::Copy( font[94], 8, 6, font[94], 4, 1, 1, 1 );
            fheroes2::Copy( font[94], 8, 6, font[94], 5, 0, 1, 1 );
            fheroes2::Copy( font[94], 4, 3, font[94], 6, 0, 1, 1 );
            fheroes2::Copy( font[94], 4, 3, font[94], 5, 1, 1, 1 );
            font[94].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[94] );

            font[95] = font[30];
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            font[3].resize( font[79].width(), font[79].height() + 2 );
            font[3].reset();
            fheroes2::Copy( font[79], 0, 0, font[3], 0, 2, font[79].width(), font[79].height() );
            font[3].setPosition( font[79].x(), font[79].y() - 2 );
            fheroes2::Copy( font[3], 2, 2, font[3], 2, 0, 1, 1 );
            fheroes2::Copy( font[3], 2, 2, font[3], 4, 0, 1, 1 );
            updateSmallFontLetterShadow( font[3] );

            font[4].resize( font[85].width(), font[85].height() + 2 );
            font[4].reset();
            fheroes2::Copy( font[85], 0, 0, font[4], 0, 2, font[85].width(), font[85].height() );
            font[4].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::Copy( font[4], 1, 2, font[4], 3, 0, 1, 1 );
            fheroes2::Copy( font[4], 1, 2, font[4], 5, 0, 1, 1 );
            updateSmallFontLetterShadow( font[4] );

            font[6].resize( font[85].width(), font[85].height() + 2 );
            font[6].reset();
            fheroes2::Copy( font[85], 0, 0, font[6], 0, 2, font[85].width(), font[85].height() );
            font[6].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::Copy( font[6], 1, 2, font[6], 4, 0, 1, 1 );
            updateSmallFontLetterShadow( font[6] );

            font[10].resize( font[65].width(), font[65].height() + 2 );
            font[10].reset();
            fheroes2::Copy( font[65], 0, 0, font[10], 0, 2, font[65].width(), font[65].height() );
            font[10].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::Copy( font[10], 2, 2, font[10], 2, 1, 1, 1 );
            fheroes2::Copy( font[10], 2, 2, font[10], 4, 1, 1, 1 );
            fheroes2::Copy( font[10], 2, 2, font[10], 3, 0, 1, 1 );
            updateSmallFontLetterShadow( font[10] );

            font[28] = font[73];
            fheroes2::FillTransform( font[28], 0, 0, font[28].width(), 2, 1 );
            fheroes2::Copy( font[28], 1, 2, font[28], 1, 0, 1, 1 );
            fheroes2::Copy( font[28], 1, 2, font[28], 3, 0, 1, 1 );
            updateSmallFontLetterShadow( font[28] );

            font[30] = font[28];

            font[32].resize( font[65].width(), font[65].height() + 2 );
            font[32].reset();
            fheroes2::Copy( font[65], 0, 0, font[32], 0, 2, font[65].width(), font[65].height() );
            font[32].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::Copy( font[32], 2, 2, font[32], 3, 0, 1, 1 );
            fheroes2::Copy( font[32], 2, 2, font[32], 4, 1, 1, 1 );
            updateSmallFontLetterShadow( font[32] );

            font[62].resize( font[67].width(), font[67].height() + 2 );
            font[62].reset();
            fheroes2::Copy( font[67], 0, 0, font[62], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[62], 3, 4, font[62], 3, 5, 1, 1 );
            fheroes2::Copy( font[62], 3, 4, font[62], 2, 6, 1, 1 );
            font[62].setPosition( font[67].x(), font[67].y() );
            updateSmallFontLetterShadow( font[62] );

            font[64].resize( font[69].width(), font[69].height() + 2 );
            font[64].reset();
            fheroes2::Copy( font[69], 0, 0, font[64], 0, 2, font[69].width(), font[69].height() );
            font[64].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[64], 2, 2, font[64], 2, 0, 1, 1 );
            fheroes2::Copy( font[64], 2, 2, font[64], 3, 1, 1, 1 );
            updateSmallFontLetterShadow( font[64] );

            font[91] = font[28];

            font[92].resize( font[69].width(), font[69].height() + 2 );
            font[92].reset();
            fheroes2::Copy( font[69], 0, 0, font[92], 0, 2, font[69].width(), font[69].height() );
            font[92].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[92], 2, 2, font[92], 3, 0, 1, 1 );
            fheroes2::Copy( font[92], 2, 2, font[92], 2, 1, 1, 1 );
            fheroes2::Copy( font[92], 2, 2, font[92], 4, 1, 1, 1 );
            updateSmallFontLetterShadow( font[92] );

            font[93] = font[28];

            font[94].resize( font[69].width(), font[69].height() + 2 );
            font[94].reset();
            fheroes2::Copy( font[69], 0, 0, font[94], 0, 2, font[69].width(), font[69].height() );
            font[94].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[94], 2, 2, font[94], 4, 0, 1, 1 );
            fheroes2::Copy( font[94], 2, 2, font[94], 3, 1, 1, 1 );
            updateSmallFontLetterShadow( font[94] );

            font[95] = font[28];
        }
    }

    // CP-1251 supports Russian, Ukranian, Belarussian, Bulgarian, Serbian Cyrillic, Macedonian and English.
    void generateCP1251Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // TODO: add support for Serbian Cyrillic and Macedonian languages by generating missing letters.

        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = icnVsSprite[icnId];

            original.resize( baseFontSize );
            original.insert( original.end(), 128, original[0] );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            size_t offset = 0;

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

            // The same letter as above but with a verical line at the top.
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
            fheroes2::Copy( font[75], 2, 2, font[234 - 32], 2, 0, 7, 4 );
            fheroes2::Copy( font[75], 1, 0, font[234 - 32], 1, 0, 3, 1 );
            fheroes2::Copy( font[75], 0, 7, font[234 - 32], 0, 4, font[75].width(), 2 );
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

            font[234 - 32].resize( font[75].width() - 2, font[75].height() - 2 );
            font[234 - 32].reset();
            fheroes2::Copy( font[75], 1, 0, font[234 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[75], 4, 1, font[234 - 32], 3, 0, 3, 3 );
            fheroes2::Copy( font[75], 5, 4, font[234 - 32], 4, 3, 2, 2 );
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
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, icnVsSprite[icnId][0] );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::FONT];

            // Inverted exclamation mark !.
            font[161 - 32].resize( font[33 - 32].width() + 1, font[33 - 32].height() + 3 );
            font[161 - 32].reset();
            fheroes2::Copy( font[33 - 32], 1, 0, font[161 - 32], 1, 3, font[33 - 32].width(), font[33 - 32].height() );
            {
                fheroes2::Sprite temp = fheroes2::Flip( font[161 - 32], false, true );
                fheroes2::Copy( temp, 0, 2, font[161 - 32], 1, 2, temp.width(), temp.height() );
            }
            font[161 - 32].setPosition( font[33 - 32].x(), font[33 - 32].y() + 2 );
            updateNormalFontLetterShadow( font[161 - 32] );

            // Inverted question mark ?.
            font[191 - 32].resize( font[63 - 32].width() + 1, font[63 - 32].height() );
            font[191 - 32].reset();
            fheroes2::Copy( font[63 - 32], 1, 0, font[191 - 32], 0, 0, font[63 - 32].width(), 11 );
            {
                fheroes2::Sprite temp = fheroes2::Flip( font[191 - 32], true, true );
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

            // A with circumflex accent. TODO Move generation of accent for further use here.
            font[194 - 32] = font[33];

            // A with tilde accent ~. TODO Move generation of accent for further use here.
            font[195 - 32] = font[33];

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
            font[199 - 32] = font[35];

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
            font[202 - 32] = font[37];

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

            // N with tilde ~. Generate accent for further use.
            // TODO: Move tilde generation code to A with tilde when adding Portuguese.
            font[209 - 32].resize( font[46].width(), font[46].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[46], 0, 0, font[209 - 32], 0, 3, font[46].width(), font[46].height() );
            fheroes2::Copy( font[37 - 32], 7, 5, font[209 - 32], 4, 0, 5, 2 );
            fheroes2::Copy( font[37 - 32], 8, 8, font[209 - 32], 8, 2, 3, 1 );
            fheroes2::Copy( font[37 - 32], 10, 7, font[209 - 32], 10, 1, 2, 1 );
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
            font[212 - 32] = font[47];

            // O with tilde accent ~.
            font[213 - 32] = font[47];

            // O with 2 dots on top.
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
            font[226 - 32] = font[65];

            // a with tilde accent ~.
            font[227 - 32] = font[65];

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
            font[231 - 32] = font[67];

            // e with grave accent `.
            font[232 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[232 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[232 - 32], 0, 3, font[69].width(), font[69].height() );
            font[232 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::Copy( font[192 - 32], 7, 0, font[232 - 32], 3, 0, 4, 2 );
            updateNormalFontLetterShadow( font[232 - 32] );

            // e with acute accent and generate the acute accent for further use.
            font[233 - 32].resize( font[69].width(), font[69].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[233 - 32], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[193 - 32], 7, 0, font[233 - 32], 3, 0, 4, 2 );
            font[233 - 32].setPosition( font[69].x(), font[69].y() - 3 );
            updateNormalFontLetterShadow( font[233 - 32] );

            // e with circumflex accent.
            font[234 - 32] = font[69];

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

            // n with tilde ~.
            font[241 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 4 );
            font[241 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[241 - 32], 0, 4, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[209 - 32], 4, 0, font[241 - 32], 1, 0, 8, 3 );
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
            font[244 - 32] = font[79];

            // o with tilde accent ~.
            font[245 - 32] = font[79];

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

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            fheroes2::FillTransform( font[75], 4, 1, 5, 8, 1 );
            fheroes2::Copy( font[43], 6, 5, font[75], 4, 7, 3, 1 );
            fheroes2::Copy( font[43], 6, 4, font[75], 4, 6, 4, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 6, 5, 3, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 7, 4, 2, 1 );
            fheroes2::Copy( font[43], 6, 6, font[75], 4, 8, 4, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateNormalFontLetterShadow( font[75] );
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = icnVsSprite[ICN::SMALFONT];

            // Inverted exclamation mark !.
            font[161 - 32].resize( font[33 - 32].width(), font[33 - 32].height() );
            font[161 - 32].reset();
            fheroes2::Copy( font[33 - 32], 1, 0, font[161 - 32], 1, 0, font[33 - 32].width(), 7 );
            {
                fheroes2::Sprite temp = fheroes2::Flip( font[161 - 32], false, true );
                fheroes2::Copy( temp, 0, 1, font[161 - 32], 0, 0, temp.width(), temp.height() );
            }
            font[161 - 32].setPosition( font[33 - 32].x(), font[33 - 32].y() + 2 );
            updateSmallFontLetterShadow( font[161 - 32] );

            // Inverted question mark ?.
            font[191 - 32].resize( font[63 - 32].width(), font[63 - 32].height() );
            font[191 - 32].reset();
            fheroes2::Copy( font[63 - 32], 1, 0, font[191 - 32], 0, 0, font[63 - 32].width(), 7 );
            {
                fheroes2::Sprite temp = fheroes2::Flip( font[191 - 32], true, true );
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

            // A with circumflex accent. TODO Move generation of accent for further use here.
            font[194 - 32] = font[33];

            // A with tilde accent ~. TODO Move generation of accent for further use here.
            font[195 - 32] = font[33];

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
            font[199 - 32] = font[35];

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
            font[202 - 32] = font[37];

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

            // N with tilde ~. Generate tilde accent for further use.
            // TODO: Move tilde accent generation to A with tilde when adding Portuguese.
            font[209 - 32].resize( font[46].width(), font[46].height() + 3 );
            font[209 - 32].reset();
            fheroes2::Copy( font[46], 0, 0, font[209 - 32], 0, 4, font[46].width(), font[46].height() );
            fheroes2::Copy( font[37 - 32], 5, 4, font[209 - 32], 4, 0, 3, 2 );
            fheroes2::Copy( font[37 - 32], 5, 4, font[209 - 32], 7, 1, 2, 2 );
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
            font[212 - 32] = font[47];

            // O with tilde accent ~.
            font[213 - 32] = font[47];

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
            font[226 - 32] = font[65];

            // a with tilde accent ~.
            font[227 - 32] = font[65];

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
            font[231 - 32] = font[67];

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
            font[234 - 32] = font[69];

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

            // n with tilde ~.
            font[241 - 32].resize( font[110 - 32].width(), font[110 - 32].height() + 4 );
            font[241 - 32].reset();
            fheroes2::Copy( font[110 - 32], 0, 0, font[241 - 32], 0, 4, font[110 - 32].width(), font[110 - 32].height() );
            fheroes2::Copy( font[209 - 32], 4, 0, font[241 - 32], 2, 0, 5, 3 );
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
            font[244 - 32] = font[79];

            // o with tilde accent ~.
            font[245 - 32] = font[79];

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

            // u with 2 dots on top.
            font[252 - 32].resize( font[85].width(), font[85].height() + 2 );
            font[252 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[252 - 32], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 6, 0, 1, 1 );
            font[252 - 32].setPosition( font[85].x(), font[85].y() - 2 );
            updateSmallFontLetterShadow( font[252 - 32] );

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            font[75].resize( 6, 8 );
            font[75].reset();
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 0, 2, 7 );
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 6, 1, 1 );
            fheroes2::Copy( font[56], 6, 0, font[75], 3, 2, 3, 3 );
            fheroes2::Copy( font[65], 2, font[65].height() - 2, font[75], 5, 6, 2, 1 );
            fheroes2::Copy( font[65], 2, 0, font[75], 4, 5, 1, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateSmallFontLetterShadow( font[75] );
        }
    }

    void generateCP1254Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, icnVsSprite[icnId][0] );
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

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            fheroes2::FillTransform( font[75], 4, 1, 5, 8, 1 );
            fheroes2::Copy( font[43], 6, 5, font[75], 4, 7, 3, 1 );
            fheroes2::Copy( font[43], 6, 4, font[75], 4, 6, 4, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 6, 5, 3, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 7, 4, 2, 1 );
            fheroes2::Copy( font[43], 6, 6, font[75], 4, 8, 4, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateNormalFontLetterShadow( font[75] );
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

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            font[75].resize( 6, 8 );
            font[75].reset();
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 0, 2, 7 );
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 6, 1, 1 );
            fheroes2::Copy( font[56], 6, 0, font[75], 3, 2, 3, 3 );
            fheroes2::Copy( font[65], 2, font[65].height() - 2, font[75], 5, 6, 2, 1 );
            fheroes2::Copy( font[65], 2, 0, font[75], 4, 5, 1, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateSmallFontLetterShadow( font[75] );
        }
    }

    void generateISO8859_16Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            icnVsSprite[icnId].resize( baseFontSize );
            icnVsSprite[icnId].insert( icnVsSprite[icnId].end(), 160, icnVsSprite[icnId][0] );
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

            // A with circonflex and generate the accent for further use.
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

            // a with circonflex.
            font[226 - 32].resize( font[65].width(), font[65].height() + 4 );
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

            // t with comma.
            font[254 - 32].resize( font[84].width(), font[84].height() + 4 );
            font[254 - 32].reset();
            fheroes2::Copy( font[84], 0, 0, font[254 - 32], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[12], 0, 0, font[254 - 32], 1, 12, font[12].width(), font[12].height() );
            font[254 - 32].setPosition( font[84].x(), font[84].y() );
            updateNormalFontLetterShadow( font[254 - 32] );

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            fheroes2::FillTransform( font[75], 4, 1, 5, 8, 1 );
            fheroes2::Copy( font[43], 6, 5, font[75], 4, 7, 3, 1 );
            fheroes2::Copy( font[43], 6, 4, font[75], 4, 6, 4, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 6, 5, 3, 1 );
            fheroes2::Copy( font[43], 7, 4, font[75], 7, 4, 2, 1 );
            fheroes2::Copy( font[43], 6, 6, font[75], 4, 8, 4, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateNormalFontLetterShadow( font[75] );
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

            // A with circonflex and generate the accent for further use.
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

            // a with circonflex.
            font[226 - 32].resize( font[65].width(), font[65].height() + 4 );
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

            // t with comma.
            font[254 - 32].resize( font[84].width(), font[84].height() + 4 );
            font[254 - 32].reset();
            fheroes2::Copy( font[84], 0, 0, font[254 - 32], 0, 0, font[84].width(), font[84].height() );
            fheroes2::Copy( font[12], 0, 0, font[254 - 32], 1, 8, font[12].width(), font[12].height() );
            font[254 - 32].setPosition( font[84].x(), font[84].y() );
            updateSmallFontLetterShadow( font[254 - 32] );

            // Proper lowercase k. Kept at end in case any letters use it for generation.
            font[75].resize( 6, 8 );
            font[75].reset();
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 0, 2, 7 );
            fheroes2::Copy( font[76], 1, 0, font[75], 1, 6, 1, 1 );
            fheroes2::Copy( font[56], 6, 0, font[75], 3, 2, 3, 3 );
            fheroes2::Copy( font[65], 2, font[65].height() - 2, font[75], 5, 6, 2, 1 );
            fheroes2::Copy( font[65], 2, 0, font[75], 4, 5, 1, 1 );
            font[75].setPosition( font[75].x(), font[75].y() );
            updateSmallFontLetterShadow( font[75] );
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

        // -
        released[13].resize( 6 + offset * 2, 6 + offset * 2 );
        released[13].reset();
        fheroes2::DrawLine( released[13], { offset + 0, offset + 5 }, { offset + 5, offset + 5 }, buttonGoodReleasedColor );

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

        // 3
        released[19].resize( 7 + offset * 2, 10 + offset * 2 );
        released[19].reset();
        fheroes2::DrawLine( released[19], { offset + 1, offset + 0 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 6, offset + 1 }, { offset + 6, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 2, offset + 4 }, { offset + 5, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 6, offset + 5 }, { offset + 6, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[19], { offset + 1, offset + 8 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[19], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[19], offset + 0, offset + 7, buttonGoodReleasedColor );

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
        released[40].resize( 14 + offset * 2, 10 + offset * 2 );
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
        released[49].resize( 13 + offset * 2, 11 + offset * 2 );
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
    }

    void generateCP1250GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        released.insert( released.end(), 160, released[0] );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        released[108].setPosition( buttonFontOffset.x, -3 );
        released[166].setPosition( buttonFontOffset.x, -3 );

        // S with caron. Only copied from S.
        released[106] = released[51];

        // S with acute accent.
        released[108].resize( released[51].width(), released[51].height() + 4 );
        released[108].reset();
        fheroes2::Copy( released[51], 0, 0, released[108], 0, 3, released[51].width(), released[51].height() );
        fheroes2::DrawLine( released[108], { offset + 4, offset + 1 }, { offset + 5, offset + 0 }, buttonGoodReleasedColor );

        // T with caron. Only copied from T.
        released[109] = released[52];

        // Z with caron. Only copied from Z.
        released[110] = released[58];

        // Z with acute. Only copied from Z.
        released[111] = released[58];

        // L with stroke. Only copied from L.
        released[131] = released[44];

        // A with ogonek. Only copied from A.
        released[133] = released[33];

        // S with cedilla. Only copied from S.
        released[138] = released[51];

        // Z with dot above. Only copied from Z.
        released[143] = released[58];

        // Y with diaerisis. Only copied from Y.
        released[156] = released[57];

        // L with caron. Only copied from L.
        released[158] = released[44];

        // R with acute. Only copied from R.
        released[160] = released[50];

        // A with acute. Only copied from A.
        released[161] = released[33];

        // A with circumflex. Only copied from A.
        released[162] = released[33];

        // A with breve. Only copied from A.
        released[163] = released[33];

        // A with diaerisis. Only copied from A.
        released[164] = released[33];

        // L with acute. Only copied from L.
        released[165] = released[44];

        // C with acute accent.
        released[166].resize( released[35].width(), released[35].height() + 4 );
        released[166].reset();
        fheroes2::Copy( released[35], 0, 0, released[166], 0, 3, released[35].width(), released[35].height() );
        fheroes2::DrawLine( released[166], { offset + 5, offset + 1 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );

        // C with cedilla. Only copied from C.
        released[167] = released[35];

        // C with caron. Only copied from C.
        released[168] = released[35];

        // E with acute. Only copied from E.
        released[169] = released[37];

        // E with ogonek. Only copied from E.
        released[170] = released[37];

        // E with diaerisis. Only copied from E.
        released[171] = released[37];

        // E with caron. Only copied from E.
        released[172] = released[37];

        // I with acute. Only copied from I.
        released[173] = released[41];

        // I with circumflex. Only copied from I.
        released[174] = released[41];

        // D with caron. Only copied from D.
        released[175] = released[36];

        // D with stroke. Only copied from D.
        released[176] = released[36];

        // N with acute. Only copied from N.
        released[177] = released[46];

        // N with caron. Only copied from N.
        released[178] = released[46];

        // O with acute. Only copied from O.
        released[179] = released[47];

        // O with circumflex. Only copied from O.
        released[180] = released[47];

        // O with double acute. Only copied from O.
        released[181] = released[47];

        // O with diaerisis. Only copied from O.
        released[182] = released[47];

        // R with caron. Only copied from R.
        released[184] = released[50];

        // U with ring above. Only copied from U.
        released[185] = released[53];

        // U with acute. Only copied from U.
        released[186] = released[53];

        // U with double acute. Only copied from U.
        released[187] = released[53];

        // U with diaerisis. Only copied from U.
        released[188] = released[53];

        // Y with acute. Only copied from Y.
        released[189] = released[57];

        // T with cedilla. Only copied from T.
        released[190] = released[52];
    }

    void generateCP1251GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        released.insert( released.end(), 160, released[0] );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        released[109].setPosition( buttonFontOffset.x, -3 );
        released[136].setPosition( buttonFontOffset.x, -3 );
        released[143].setPosition( buttonFontOffset.x, -3 );
        released[169].setPosition( buttonFontOffset.x, -3 );

        // K with acute, Cyrillic KJE. Needs to have upper right arm adjusted.
        released[109].resize( released[43].width(), released[43].height() + 4 );
        released[109].reset();
        fheroes2::Copy( released[43], 0, 0, released[109], 0, 3, released[43].width(), released[43].height() );
        fheroes2::DrawLine( released[109], { offset + 6, offset + 1 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );

        // J
        released[131] = released[42];

        // E with two dots above.
        released[136].resize( released[37].width(), released[37].height() + 3 );
        released[136].reset();
        fheroes2::Copy( released[37], 0, 0, released[136], 0, 3, released[37].width(), released[37].height() );
        fheroes2::SetPixel( released[136], offset + 3, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[136], offset + 6, offset + 1, buttonGoodReleasedColor );

        // I with two dots above, Cyrillic YI
        released[143].resize( released[41].width(), released[41].height() + 3 );
        released[143].reset();
        fheroes2::Copy( released[41], 0, 0, released[143], 0, 3, released[41].width(), released[41].height() );
        fheroes2::SetPixel( released[143], offset + 1, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[143], offset + 4, offset + 1, buttonGoodReleasedColor );

        // I, Belarusian-Ukrainian I
        released[146] = released[41];

        // S
        released[157] = released[51];

        // A
        released[160] = released[33];

        // r with small circle, Cyrillic BE
        released[161].resize( 10 + offset * 2, 10 + offset * 2 );
        released[161].reset();
        fheroes2::DrawLine( released[161], { offset + 0, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 3, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[161], { offset + 9, offset + 6 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );

        // B
        released[162] = released[34];

        // r, Cyrillic GHE
        released[163].resize( 10 + offset * 2, 10 + offset * 2 );
        released[163].reset();
        fheroes2::DrawLine( released[163], { offset + 0, offset + 0 }, { offset + 9, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[163], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[163], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[163], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );

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
        released[165] = released[37];

        // X with vertical stroke through it, Cyrillic ZHE. Needs to have upper right and left arms adjusted.
        released[166].resize( released[43].width() + 5 + offset, released[43].height() + offset * 2 );
        released[166].reset();
        fheroes2::Copy( released[43], 0, 0, released[166], 7, 0, released[43].width(), released[43].height() );
        fheroes2::DrawLine( released[166], { offset + 1, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 6, offset + 4 }, { offset + 8, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 3, offset + 1 }, { offset + 5, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 2, offset + 8 }, { offset + 5, offset + 5 }, buttonGoodReleasedColor );

        // 3, Cyrillic ZE
        released[167].resize( released[19].width() + 1, released[19].height() );
        released[167].reset();
        fheroes2::DrawLine( released[167], { offset + 1, offset + 0 }, { offset + 6, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 7, offset + 1 }, { offset + 7, offset + 3 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 2, offset + 4 }, { offset + 6, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 7, offset + 5 }, { offset + 7, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[167], { offset + 1, offset + 8 }, { offset + 6, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[167], offset + 0, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[167], offset + 0, offset + 7, buttonGoodReleasedColor );

        // Mirrored N, Cyrillic I
        released[168].resize( released[46].width(), released[46].height() );
        released[168].reset();
        fheroes2::Flip( released[46], 0, 0, released[168], 0, 0, released[46].width(), released[46].height(), true, false );

        // Mirrored N with breve, Cyrillic Short I
        released[169].resize( released[168].width(), released[168].height() + 4 );
        released[169].reset();
        fheroes2::Copy( released[168], 0, 0, released[169], 0, 3, released[168].width(), released[168].height() );
        fheroes2::DrawLine( released[169], { offset + 6, offset + 2 }, { offset + 8, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[169], offset + 5, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[169], offset + 9, offset + 1, buttonGoodReleasedColor );

        // K. Needs to have upper right arm adjusted.
        released[170] = released[43];

        // Cyrillic EL
        released[171].resize( 9 + offset * 2, 10 + offset * 2 );
        released[171].reset();
        fheroes2::DrawLine( released[171], { offset + 2, offset + 0 }, { offset + 8, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 8, offset + 1 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 3, offset + 1 }, { offset + 3, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 0, offset + 9 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[171], { offset + 6, offset + 9 }, { offset + 7, offset + 9 }, buttonGoodReleasedColor );

        // M
        released[172] = released[45];

        // H
        released[173] = released[40];

        // O
        released[174] = released[47];

        // P
        released[176] = released[48];

        // C
        released[177] = released[35];

        // T
        released[178] = released[52];

        // y, Cyrillic U
        released[179].resize( 11 + offset * 2, 10 + offset * 2 );
        released[179].reset();
        fheroes2::DrawLine( released[179], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 2, offset + 1 }, { offset + 4, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 6, offset + 5 }, { offset + 8, offset + 1 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 5, offset + 5 }, { offset + 5, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[179], { offset + 2, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[179], offset + 4, offset + 5, buttonGoodReleasedColor );

        // X
        released[181] = released[56];

        // b, Cyrillic hard sign
        released[186].resize( 11 + offset * 2, 10 + offset * 2 );
        released[186].reset();
        fheroes2::DrawLine( released[186], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 1, offset + 9 }, { offset + 9, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 3, offset + 1 }, { offset + 3, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 4, offset + 5 }, { offset + 9, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[186], { offset + 10, offset + 6 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );

        // bI, Cyrillic YERU
        released[187].resize( 17 + offset * 2, 10 + offset * 2 );
        released[187].reset();
        fheroes2::DrawLine( released[187], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 3, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[187], { offset + 9, offset + 6 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::Copy( released[41], 0, 0, released[187], 12, 0, released[41].width(), released[41].height() );

        // b, Cyrillic soft sign
        released[188].resize( 10 + offset * 2, 10 + offset * 2 );
        released[188].reset();
        fheroes2::DrawLine( released[188], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 3, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[188], { offset + 9, offset + 6 }, { offset + 9, offset + 8 }, buttonGoodReleasedColor );

        // IO, Cyrillic YU
        released[190].resize( released[41].width() + released[47].width(), 10 + offset * 2 );
        released[190].reset();
        fheroes2::Copy( released[41], 0, 0, released[190], 0, 0, released[41].width(), released[41].height() );
        fheroes2::Copy( released[47], 0, 0, released[190], released[41].width() - 1, 0, released[47].width(), released[47].height() );
        fheroes2::DrawLine( released[190], { offset + 3, offset + 4 }, { offset + 7, offset + 4 }, buttonGoodReleasedColor );

        // Mirrored R, Cyrillic YA
        released[191].resize( released[50].width(), released[50].height() );
        released[191].reset();
        fheroes2::Flip( released[50], 0, 0, released[191], 0, 0, released[50].width(), released[50].height(), true, false );
    }

    void generateCP1252GoodButtonFont( std::vector<fheroes2::Sprite> & released )
    {
        // Increase size to fit full CP1252 set of characters. Fill with 1px transparent images.
        released.insert( released.end(), 160, released[0] );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Offset letters with diacritics above them.
        released[165].setPosition( buttonFontOffset.x, -2 );

        // A with circle on top
        released[165].resize( 13 + offset * 2, 12 + offset * 2 );
        released[165].reset();
        fheroes2::DrawLine( released[165], { offset + 0, offset + 11 }, { offset + 4, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 8, offset + 11 }, { offset + 12, offset + 11 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 5, offset + 7 }, { offset + 8, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 2, offset + 10 }, { offset + 4, offset + 7 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 7, offset + 3 }, { offset + 10, offset + 10 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 5, offset + 1 }, { offset + 5, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 8, offset + 1 }, { offset + 8, offset + 2 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[165], { offset + 6, offset + 0 }, { offset + 7, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[165], offset + 4, offset + 6, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[165], offset + 5, offset + 5, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[165], offset + 5, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[165], offset + 6, offset + 3, buttonGoodReleasedColor );

        // A attached to E.
        released[166].resize( 18 + offset * 2, 12 + offset * 2 );
        released[166].reset();
        fheroes2::DrawLine( released[166], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 8, offset + 9 }, { offset + 12, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 5, offset + 5 }, { offset + 8, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 2, offset + 8 }, { offset + 4, offset + 5 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 7, offset + 1 }, { offset + 10, offset + 8 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 7, offset + 0 }, { offset + 14, offset + 0 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 13, offset + 9 }, { offset + 14, offset + 9 }, buttonGoodReleasedColor );
        fheroes2::DrawLine( released[166], { offset + 9, offset + 4 }, { offset + 12, offset + 4 }, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 4, offset + 4, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 5, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 5, offset + 2, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 6, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 6, offset + 0, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 14, offset + 1, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 14, offset + 8, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 12, offset + 3, buttonGoodReleasedColor );
        fheroes2::SetPixel( released[166], offset + 12, offset + 5, buttonGoodReleasedColor );
    }
}

namespace fheroes2
{
    void generateAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite )
    {
        switch ( language ) {
        case SupportedLanguage::Hungarian:
        case SupportedLanguage::Polish:
            generateCP1250Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::French:
            generateFrenchAlphabet( icnVsSprite );
            break;
        case SupportedLanguage::Belarusian:
        case SupportedLanguage::Bulgarian:
        case SupportedLanguage::Russian:
        case SupportedLanguage::Ukrainian:
            generateCP1251Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::Dutch:
        case SupportedLanguage::German:
        case SupportedLanguage::Italian:
        case SupportedLanguage::Norwegian:
        case SupportedLanguage::Portuguese:
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
            generateCP1252Alphabet( icnVsSprite );
            break;
        case SupportedLanguage::Turkish:
            generateCP1254Alphabet( icnVsSprite );
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

        // NOTE: As soon as code structure is agreed on functions for all Code Pages will be added.
        switch ( language ) {
        case SupportedLanguage::English:
            generateBaseButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED], icnVsSprite[ICN::BUTTON_GOOD_FONT_PRESSED], icnVsSprite[ICN::BUTTON_EVIL_FONT_RELEASED],
                                    icnVsSprite[ICN::BUTTON_EVIL_FONT_PRESSED] );
            return;
        case SupportedLanguage::Hungarian:
        case SupportedLanguage::Polish:
            generateCP1250GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::French:
            // TODO: Adjust French to use CP1252 button font.
            // Currently all French capital letters are replaced with the Latin counterpart during .mo compilation.
            // Therefore, there is no need to generate a French specific button alphabet.
            // generateFrenchGoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Belarusian:
        case SupportedLanguage::Bulgarian:
        case SupportedLanguage::Russian:
        case SupportedLanguage::Ukrainian:
            generateCP1251GoodButtonFont( icnVsSprite[ICN::BUTTON_GOOD_FONT_RELEASED] );
            break;
        case SupportedLanguage::Dutch:
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
        assert( !icnVsSprite.empty() );

        // Proper lowercase k.
        fheroes2::FillTransform( icnVsSprite[75], 4, 1, 5, 8, 1 );
        fheroes2::Copy( icnVsSprite[43], 6, 5, icnVsSprite[75], 4, 7, 3, 1 );
        fheroes2::Copy( icnVsSprite[43], 6, 4, icnVsSprite[75], 4, 6, 4, 1 );
        fheroes2::Copy( icnVsSprite[43], 7, 4, icnVsSprite[75], 6, 5, 3, 1 );
        fheroes2::Copy( icnVsSprite[43], 7, 4, icnVsSprite[75], 7, 4, 2, 1 );
        fheroes2::Copy( icnVsSprite[43], 6, 6, icnVsSprite[75], 4, 8, 4, 1 );
        icnVsSprite[75].setPosition( icnVsSprite[75].x(), icnVsSprite[75].y() );
        updateNormalFontLetterShadow( icnVsSprite[75] );
    }

    void modifyBaseSmallFont( std::vector<fheroes2::Sprite> & icnVsSprite )
    {
        assert( !icnVsSprite.empty() );

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
}
