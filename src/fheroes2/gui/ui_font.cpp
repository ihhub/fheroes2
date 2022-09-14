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

    void generateCP1250Alphabet( std::vector<std::vector<fheroes2::Sprite>> & icnVsSprite )
    {
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = icnVsSprite[icnId];

            original.resize( baseFontSize );
            original.insert( original.end(), 128, original[0] );
            original[140 - 32] = original[83 - 32];
            original[143 - 32] = original[90 - 32];
            original[156 - 32] = original[115 - 32];
            original[159 - 32] = original[122 - 32];
            original[163 - 32] = original[76 - 32];
            original[165 - 32] = original[65 - 32];
            // Uppercase S with cedilla
            original[170 - 32] = original[83 - 32];
            original[175 - 32] = original[90 - 32];
            original[179 - 32] = original[108 - 32];
            original[185 - 32] = original[97 - 32];
            // Lowercase s with cedilla
            original[186 - 32] = original[115 - 32];
            original[191 - 32] = original[122 - 32];
            // Uppercase A with circumflex
            original[194 - 32] = original[65 - 32];
            // Uppercase A with breve
            original[195 - 32] = original[65 - 32];
            original[198 - 32] = original[67 - 32];
            original[202 - 32] = original[69 - 32];
            // Uppercase I with circumflex
            original[206 - 32] = original[73 - 32];
            original[209 - 32] = original[78 - 32];
            original[211 - 32] = original[79 - 32];
            // Uppercase T with cedilla
            original[222 - 32] = original[84 - 32];
            // Lowercase a with circumflex
            original[226 - 32] = original[97 - 32];
            // Lowercase a with breve
            original[227 - 32] = original[97 - 32];
            original[230 - 32] = original[99 - 32];
            original[234 - 32] = original[101 - 32];
            // Lowercase i with circumflex
            original[238 - 32] = original[105 - 32];
            original[241 - 32] = original[110 - 32];
            original[243 - 32] = original[111 - 32];
            // Lowercase t with cedilla
            original[254 - 32] = original[116 - 32];
        }

        // TODO: modify newly added characters accordingly.
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
            {
                // TODO: add proper Flip function variant.
                fheroes2::Sprite temp = fheroes2::Crop( font[48], 6, 0, 5, 6 );
                temp = fheroes2::Flip( temp, true, false );
                fheroes2::Copy( temp, 0, 0, font[212 - 32], 1, 0, temp.width(), temp.height() );
            }
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
            // Italian uses CP1252 for special characters so we need to extend the array.
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
}

namespace fheroes2
{
    void generateAlphabet( const SupportedLanguage language, std::vector<std::vector<Sprite>> & icnVsSprite )
    {
        switch ( language ) {
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
        case SupportedLanguage::German:
        case SupportedLanguage::Italian:
        case SupportedLanguage::Norwegian:
        case SupportedLanguage::Portuguese:
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
            generateCP1252Alphabet( icnVsSprite );
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
        case SupportedLanguage::Spanish:
        case SupportedLanguage::Swedish:
        case SupportedLanguage::Ukrainian:
            return true;
        default:
            break;
        }

        return false;
    }

    void generateBaseButtonFont( std::vector<Sprite> & released, std::vector<Sprite> & pressed, const uint8_t releasedFontColor, const uint8_t pressedFontColor,
                                 const uint8_t releasedContourColor )
    {
        // Button font does not exist in the original game assets but we can regenerate it from scratch.
        // All letters in buttons have some variations in colors but overall shapes are the same.
        // We want to standartize the font and to use one approach to generate letters.
        // The shape of the letter is defined only by one color (in general). The rest of information is generated from transformations and contours.
        //
        // Another essential difference from normal fonts is that button font has only uppercase letters.
        // This means that we need to generate only 26 letter of English alphabet, 10 digits and few special characters, totalling in about 50 symbols.
        // The downside of this font is that we have to make released and pressed states of each letter.

        // Generate the shape of letters and then apply all effects.
        released.resize( baseFontSize );

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // 0
        released[16].resize( 9 + offset * 2, 10 + offset * 2 );
        released[16].reset();
        DrawLine( released[16], { offset + 2, offset + 0 }, { offset + 6, offset + 0 }, releasedFontColor );
        DrawLine( released[16], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, releasedFontColor );
        DrawLine( released[16], { offset + 2, offset + 9 }, { offset + 6, offset + 9 }, releasedFontColor );
        DrawLine( released[16], { offset + 8, offset + 2 }, { offset + 8, offset + 7 }, releasedFontColor );
        SetPixel( released[16], offset + 1, offset + 1, releasedFontColor );
        SetPixel( released[16], offset + 7, offset + 1, releasedFontColor );
        SetPixel( released[16], offset + 1, offset + 8, releasedFontColor );
        SetPixel( released[16], offset + 7, offset + 8, releasedFontColor );

        // 1
        released[17].resize( 5 + offset * 2, 10 + offset * 2 );
        released[17].reset();
        DrawLine( released[17], { offset + 2, offset + 0 }, { offset + 2, offset + 9 }, releasedFontColor );
        DrawLine( released[17], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, releasedFontColor );
        SetPixel( released[17], offset + 1, offset + 1, releasedFontColor );
        SetPixel( released[17], offset + 0, offset + 2, releasedFontColor );

        // C
        released[35].resize( 10 + offset * 2, 10 + offset * 2 );
        released[35].reset();
        DrawLine( released[35], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, releasedFontColor );
        DrawLine( released[35], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, releasedFontColor );
        DrawLine( released[35], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, releasedFontColor );
        DrawLine( released[35], { offset + 9, offset + 0 }, { offset + 9, offset + 2 }, releasedFontColor );
        SetPixel( released[35], offset + 1, offset + 1, releasedFontColor );
        SetPixel( released[35], offset + 1, offset + 8, releasedFontColor );
        SetPixel( released[35], offset + 8, offset + 1, releasedFontColor );
        SetPixel( released[35], offset + 8, offset + 8, releasedFontColor );
        SetPixel( released[35], offset + 9, offset + 7, releasedFontColor );

        // D
        released[36].resize( 11 + offset * 2, 10 + offset * 2 );
        released[36].reset();
        DrawLine( released[36], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, releasedFontColor );
        DrawLine( released[36], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, releasedFontColor );
        DrawLine( released[36], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, releasedFontColor );
        DrawLine( released[36], { offset + 10, offset + 2 }, { offset + 10, offset + 7 }, releasedFontColor );
        SetPixel( released[36], offset + 9, offset + 1, releasedFontColor );
        SetPixel( released[36], offset + 9, offset + 8, releasedFontColor );

        // F
        released[38].resize( 9 + offset * 2, 10 + offset * 2 );
        released[38].reset();
        DrawLine( released[38], { offset + 0, offset + 0 }, { offset + 8, offset + 0 }, releasedFontColor );
        DrawLine( released[38], { offset + 0, offset + 9 }, { offset + 3, offset + 9 }, releasedFontColor );
        DrawLine( released[38], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, releasedFontColor );
        DrawLine( released[38], { offset + 3, offset + 4 }, { offset + 6, offset + 4 }, releasedFontColor );
        SetPixel( released[38], offset + 8, offset + 1, releasedFontColor );
        SetPixel( released[38], offset + 6, offset + 3, releasedFontColor );
        SetPixel( released[38], offset + 6, offset + 5, releasedFontColor );

        // I
        released[41].resize( 5 + offset * 2, 10 + offset * 2 );
        released[41].reset();
        DrawLine( released[41], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, releasedFontColor );
        DrawLine( released[41], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, releasedFontColor );
        DrawLine( released[41], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, releasedFontColor );

        // K
        released[43].resize( 12 + offset * 2, 10 + offset * 2 );
        released[43].reset();
        DrawLine( released[43], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, releasedFontColor );
        DrawLine( released[43], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, releasedFontColor );
        DrawLine( released[43], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, releasedFontColor );
        DrawLine( released[43], { offset + 3, offset + 4 }, { offset + 5, offset + 4 }, releasedFontColor );
        DrawLine( released[43], { offset + 6, offset + 3 }, { offset + 8, offset + 1 }, releasedFontColor );
        DrawLine( released[43], { offset + 6, offset + 5 }, { offset + 9, offset + 8 }, releasedFontColor );
        DrawLine( released[43], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, releasedFontColor );
        DrawLine( released[43], { offset + 8, offset + 9 }, { offset + 11, offset + 9 }, releasedFontColor );

        // L
        released[44].resize( 9 + offset * 2, 10 + offset * 2 );
        released[44].reset();
        DrawLine( released[44], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, releasedFontColor );
        DrawLine( released[44], { offset + 0, offset + 9 }, { offset + 8, offset + 9 }, releasedFontColor );
        DrawLine( released[44], { offset + 2, offset + 1 }, { offset + 2, offset + 8 }, releasedFontColor );
        SetPixel( released[44], offset + 8, offset + 8, releasedFontColor );

        // M
        released[45].resize( 15 + offset * 2, 10 + offset * 2 );
        released[45].reset();
        DrawLine( released[45], { offset + 0, offset + 0 }, { offset + 2, offset + 0 }, releasedFontColor );
        DrawLine( released[45], { offset + 2, offset + 0 }, { offset + 2, offset + 8 }, releasedFontColor );
        DrawLine( released[45], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, releasedFontColor );
        DrawLine( released[45], { offset + 3, offset + 1 }, { offset + 7, offset + 5 }, releasedFontColor );
        DrawLine( released[45], { offset + 8, offset + 4 }, { offset + 11, offset + 1 }, releasedFontColor );
        DrawLine( released[45], { offset + 12, offset + 1 }, { offset + 12, offset + 8 }, releasedFontColor );
        DrawLine( released[45], { offset + 12, offset + 0 }, { offset + 14, offset + 0 }, releasedFontColor );
        DrawLine( released[45], { offset + 10, offset + 9 }, { offset + 14, offset + 9 }, releasedFontColor );

        // N
        released[46].resize( 14 + offset * 2, 10 + offset * 2 );
        released[46].reset();
        DrawLine( released[46], { offset + 0, offset + 0 }, { offset + 1, offset + 0 }, releasedFontColor );
        DrawLine( released[46], { offset + 2, offset + 0 }, { offset + 2, offset + 8 }, releasedFontColor );
        DrawLine( released[46], { offset + 0, offset + 9 }, { offset + 4, offset + 9 }, releasedFontColor );
        DrawLine( released[46], { offset + 3, offset + 1 }, { offset + 10, offset + 8 }, releasedFontColor );
        DrawLine( released[46], { offset + 9, offset + 0 }, { offset + 13, offset + 0 }, releasedFontColor );
        DrawLine( released[46], { offset + 11, offset + 0 }, { offset + 11, offset + 9 }, releasedFontColor );

        // O
        released[47].resize( 10 + offset * 2, 10 + offset * 2 );
        released[47].reset();
        DrawLine( released[47], { offset + 2, offset + 0 }, { offset + 7, offset + 0 }, releasedFontColor );
        DrawLine( released[47], { offset + 0, offset + 2 }, { offset + 0, offset + 7 }, releasedFontColor );
        DrawLine( released[47], { offset + 2, offset + 9 }, { offset + 7, offset + 9 }, releasedFontColor );
        DrawLine( released[47], { offset + 9, offset + 2 }, { offset + 9, offset + 7 }, releasedFontColor );
        SetPixel( released[47], offset + 1, offset + 1, releasedFontColor );
        SetPixel( released[47], offset + 8, offset + 1, releasedFontColor );
        SetPixel( released[47], offset + 1, offset + 8, releasedFontColor );
        SetPixel( released[47], offset + 8, offset + 8, releasedFontColor );

        // T
        released[52].resize( 11 + offset * 2, 10 + offset * 2 );
        released[52].reset();
        DrawLine( released[52], { offset + 0, offset + 0 }, { offset + 10, offset + 0 }, releasedFontColor );
        DrawLine( released[52], { offset + 5, offset + 1 }, { offset + 5, offset + 8 }, releasedFontColor );
        DrawLine( released[52], { offset + 0, offset + 1 }, { offset + 0, offset + 2 }, releasedFontColor );
        DrawLine( released[52], { offset + 10, offset + 1 }, { offset + 10, offset + 2 }, releasedFontColor );
        DrawLine( released[52], { offset + 4, offset + 9 }, { offset + 6, offset + 9 }, releasedFontColor );

        // U
        released[53].resize( 13 + offset * 2, 10 + offset * 2 );
        released[53].reset();
        DrawLine( released[53], { offset + 0, offset + 0 }, { offset + 4, offset + 0 }, releasedFontColor );
        DrawLine( released[53], { offset + 8, offset + 0 }, { offset + 12, offset + 0 }, releasedFontColor );
        DrawLine( released[53], { offset + 2, offset + 1 }, { offset + 2, offset + 7 }, releasedFontColor );
        DrawLine( released[53], { offset + 10, offset + 1 }, { offset + 10, offset + 7 }, releasedFontColor );
        DrawLine( released[53], { offset + 4, offset + 9 }, { offset + 8, offset + 9 }, releasedFontColor );
        SetPixel( released[53], offset + 3, offset + 8, releasedFontColor );
        SetPixel( released[53], offset + 9, offset + 8, releasedFontColor );

        // Y
        released[57].resize( 11 + offset * 2, 10 + offset * 2 );
        released[57].reset();
        DrawLine( released[57], { offset + 0, offset + 0 }, { offset + 3, offset + 0 }, releasedFontColor );
        DrawLine( released[57], { offset + 7, offset + 0 }, { offset + 10, offset + 0 }, releasedFontColor );
        DrawLine( released[57], { offset + 2, offset + 1 }, { offset + 4, offset + 3 }, releasedFontColor );
        DrawLine( released[57], { offset + 6, offset + 3 }, { offset + 8, offset + 1 }, releasedFontColor );
        DrawLine( released[57], { offset + 5, offset + 4 }, { offset + 5, offset + 8 }, releasedFontColor );
        DrawLine( released[57], { offset + 3, offset + 9 }, { offset + 7, offset + 9 }, releasedFontColor );

        pressed = released;

        // Apply all special effects.
        for ( Sprite & letter : released ) {
            updateShadow( letter, { 1, -1 }, 2 );
            updateShadow( letter, { 2, -2 }, 4 );
            letter = addContour( letter, { -1, 1 }, releasedContourColor );
            updateShadow( letter, { -1, 1 }, 7 );
        }

        for ( Sprite & letter : pressed ) {
            ReplaceColorId( letter, releasedFontColor, pressedFontColor );

            fheroes2::updateShadow( letter, { 1, -1 }, 2 );
            fheroes2::updateShadow( letter, { -1, 1 }, 7 );
            fheroes2::updateShadow( letter, { -2, 2 }, 8 );
        }
    }
}
