/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021 - 2022                                             *
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

#include <array>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>

#include "agg.h"
#include "agg_file.h"
#include "agg_image.h"
#include "h2d.h"
#include "icn.h"
#include "image.h"
#include "image_tool.h"
#include "pal.h"
#include "screen.h"
#include "text.h"
#include "til.h"
#include "tools.h"
#include "ui_language.h"
#include "ui_text.h"

namespace
{
    const std::array<const char *, TIL::LASTTIL> tilFileName = { "UNKNOWN", "CLOF32.TIL", "GROUND32.TIL", "STON.TIL" };

    std::vector<std::vector<fheroes2::Sprite>> _icnVsSprite( ICN::LASTICN );
    std::vector<std::vector<std::vector<fheroes2::Image>>> _tilVsImage( TIL::LASTTIL );
    const fheroes2::Sprite errorImage;

    const uint32_t headerSize = 6;

    std::map<int, std::vector<fheroes2::Sprite>> _icnVsScaledSprite;

    bool IsValidICNId( int id )
    {
        return id >= 0 && static_cast<size_t>( id ) < _icnVsSprite.size();
    }

    bool IsValidTILId( int id )
    {
        return id >= 0 && static_cast<size_t>( id ) < _tilVsImage.size();
    }

    fheroes2::Image createDigit( const int32_t width, const int32_t height, const std::vector<fheroes2::Point> & points )
    {
        fheroes2::Image digit( width, height );
        digit.reset();

        fheroes2::SetPixel( digit, points, 115 );
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
        for ( size_t i = 0; i < digits.size(); ++i ) {
            output.emplace_back( addDigit( origin, digits[i], offset ) );
            output.back().setPosition( output.back().width() - origin.width(), output.back().height() - origin.height() );
        }
    }

    void replaceTranformPixel( fheroes2::Image & image, const int32_t position, const uint8_t value )
    {
        if ( image.transform()[position] != 0 ) {
            image.transform()[position] = 0;
            image.image()[position] = value;
        }
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

            _normalFont = _icnVsSprite[ICN::FONT];
            _smallFont = _icnVsSprite[ICN::SMALFONT];

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

            // Clear modified fonts.
            _icnVsSprite[ICN::YELLOW_FONT].clear();
            _icnVsSprite[ICN::YELLOW_SMALLFONT].clear();
            _icnVsSprite[ICN::GRAY_FONT].clear();
            _icnVsSprite[ICN::GRAY_SMALL_FONT].clear();
            _icnVsSprite[ICN::WHITE_LARGE_FONT].clear();
        }

    private:
        bool _isPreserved = false;

        std::vector<fheroes2::Sprite> _normalFont;
        std::vector<fheroes2::Sprite> _smallFont;
    };

    OriginalAlphabetPreserver alphabetPreserver;

    void generatePolishAlphabet()
    {
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = _icnVsSprite[icnId];

            original.resize( 96 );
            original.insert( original.end(), 128, original[0] );
            original[108] = original[51];
            original[111] = original[58];
            original[124] = original[83];
            original[127] = original[90];
            original[131] = original[44];
            original[133] = original[33];
            original[143] = original[58];
            original[147] = original[76];
            original[153] = original[65];
            original[159] = original[90];
            original[166] = original[35];
            original[170] = original[37];
            original[177] = original[46];
            original[179] = original[47];
            original[198] = original[67];
            original[202] = original[69];
            original[209] = original[78];
            original[211] = original[79];
        }

        // TODO: modify newly added characters accordingly.
    }

    void generateGermanAlphabet()
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            _icnVsSprite[icnId].resize( 96 );
            _icnVsSprite[icnId].insert( _icnVsSprite[icnId].end(), 160, _icnVsSprite[icnId][0] );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::FONT];

            // A with 2 dots on top.
            font[196 - 32].resize( font[33].width(), font[33].height() + 3 );
            font[196 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[196 - 32], 0, 3, font[33].width(), font[33].height() );
            fheroes2::Copy( font[196 - 32], 3, 1 + 3, font[196 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[196 - 32], 4, 1 + 3, font[196 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[196 - 32], 4, 0, font[196 - 32], 9, 0, 2, 1 );
            font[196 - 32].setPosition( font[33].x(), font[33].y() - 3 );
            fheroes2::updateShadow( font[196 - 32], { -1, 2 }, 2 );

            // O with 2 dots on top.
            font[214 - 32].resize( font[47].width(), font[47].height() + 3 );
            font[214 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[214 - 32], 0, 3, font[47].width(), font[47].height() );
            fheroes2::Copy( font[214 - 32], 1, 2 + 3, font[214 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 2, 2 + 3, font[214 - 32], 6, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 5, 0, font[214 - 32], 10, 0, 2, 1 );
            font[214 - 32].setPosition( font[47].x(), font[47].y() - 3 );
            fheroes2::updateShadow( font[214 - 32], { -1, 2 }, 2 );

            // U with 2 dots on top.
            font[220 - 32].resize( font[53].width(), font[53].height() + 3 );
            font[220 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[220 - 32], 0, 3, font[53].width(), font[53].height() );
            fheroes2::Copy( font[220 - 32], 1, 1 + 3, font[220 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 2, 1 + 3, font[220 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 4, 0, font[220 - 32], 9, 0, 2, 1 );
            font[220 - 32].setPosition( font[53].x(), font[53].y() - 3 );
            fheroes2::updateShadow( font[220 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[228 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[246 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[252 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[223 - 32], { -1, 2 }, 2 );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::SMALFONT];

            // A with 2 dots on top.
            font[196 - 32].resize( font[33].width(), font[33].height() + 2 );
            font[196 - 32].reset();
            fheroes2::Copy( font[33], 0, 0, font[196 - 32], 0, 2, font[33].width(), font[33].height() );
            fheroes2::Copy( font[196 - 32], 3, 0 + 2, font[196 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[196 - 32], 3, 0 + 2, font[196 - 32], 6, 0, 1, 1 );
            font[196 - 32].setPosition( font[33].x(), font[33].y() - 2 );
            fheroes2::updateShadow( font[196 - 32], { -1, 1 }, 2 );

            // O with 2 dots on top.
            font[214 - 32].resize( font[47].width(), font[47].height() + 2 );
            font[214 - 32].reset();
            fheroes2::Copy( font[47], 0, 0, font[214 - 32], 0, 2, font[47].width(), font[47].height() );
            fheroes2::Copy( font[214 - 32], 3, 0 + 2, font[214 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[214 - 32], 3, 0 + 2, font[214 - 32], 5, 0, 1, 1 );
            font[214 - 32].setPosition( font[47].x(), font[47].y() - 2 );
            fheroes2::updateShadow( font[214 - 32], { -1, 1 }, 2 );

            // U with 2 dots on top.
            font[220 - 32].resize( font[53].width(), font[53].height() + 2 );
            font[220 - 32].reset();
            fheroes2::Copy( font[53], 0, 0, font[220 - 32], 0, 2, font[53].width(), font[53].height() );
            fheroes2::Copy( font[220 - 32], 3, 0 + 2, font[220 - 32], 4, 0, 1, 1 );
            fheroes2::Copy( font[220 - 32], 3, 0 + 2, font[220 - 32], 6, 0, 1, 1 );
            font[220 - 32].setPosition( font[53].x(), font[53].y() - 2 );
            fheroes2::updateShadow( font[220 - 32], { -1, 1 }, 2 );

            // a with 2 dots on top.
            font[228 - 32].resize( font[65].width(), font[65].height() + 2 );
            font[228 - 32].reset();
            fheroes2::Copy( font[65], 0, 0, font[228 - 32], 0, 2, font[65].width(), font[65].height() );
            fheroes2::Copy( font[228 - 32], 3, 0 + 2, font[228 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[228 - 32], 3, 0 + 2, font[228 - 32], 5, 0, 1, 1 );
            font[228 - 32].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::updateShadow( font[228 - 32], { -1, 1 }, 2 );

            // o with 2 dots on top.
            font[246 - 32].resize( font[79].width(), font[79].height() + 2 );
            font[246 - 32].reset();
            fheroes2::Copy( font[79], 0, 0, font[246 - 32], 0, 2, font[79].width(), font[79].height() );
            fheroes2::Copy( font[246 - 32], 3, 0 + 2, font[246 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[246 - 32], 3, 0 + 2, font[246 - 32], 4, 0, 1, 1 );
            font[246 - 32].setPosition( font[79].x(), font[79].y() - 2 );
            fheroes2::updateShadow( font[246 - 32], { -1, 1 }, 2 );

            // u with 2 dots on top.
            font[252 - 32].resize( font[85].width(), font[85].height() + 2 );
            font[252 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[252 - 32], 0, 2, font[85].width(), font[85].height() );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[252 - 32], 2, 0 + 2, font[252 - 32], 6, 0, 1, 1 );
            font[252 - 32].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::updateShadow( font[252 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[223 - 32], { -1, 1 }, 2 );
        }
    }

    void generateFrenchAlphabet()
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            _icnVsSprite[icnId].resize( 96 );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::FONT];

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
            fheroes2::updateShadow( font[3], { -1, 2 }, 2 );

            font[4].resize( font[85].width(), font[85].height() + 3 );
            font[4].reset();
            fheroes2::Copy( font[85], 0, 0, font[4], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[3], 2, 0, font[4], 3, 0, 5, 2 );
            font[4].setPosition( font[85].x(), font[85].y() - 3 );
            fheroes2::updateShadow( font[4], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[6], { -1, 2 }, 2 );

            font[10].resize( font[65].width(), font[65].height() + 3 );
            font[10].reset();
            fheroes2::Copy( font[65], 0, 0, font[10], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[3], 2, 0, font[10], 2, 0, 5, 2 );
            font[10].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::updateShadow( font[10], { -1, 2 }, 2 );

            font[28] = font[73];
            // Just to be safe and not to write something out of buffer.
            if ( font[28].width() > 2 && font[28].height() > 1 ) {
                font[28].image()[2] = 0;
                font[28].transform()[2] = 1;
                font[28].image()[2 + font[28].width()] = 0;
                font[28].transform()[2 + font[28].width()] = 1;
            }
            fheroes2::Copy( font[28], 3, 0, font[28], 1, 0, 1, 2 );
            fheroes2::updateShadow( font[28], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[30], { -1, 2 }, 2 );

            font[32].resize( font[65].width(), font[65].height() + 3 );
            font[32].reset();
            fheroes2::Copy( font[65], 0, 0, font[32], 0, 3, font[65].width(), font[65].height() );
            font[32].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[6], 4, 0, font[32], 3, 0, 4, 2 );
            fheroes2::updateShadow( font[32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[62], { -1, 2 }, 2 );

            font[64].resize( font[69].width(), font[69].height() + 3 );
            font[64].reset();
            fheroes2::Copy( font[69], 0, 0, font[64], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[64], 4, 3, font[64], 4, 0, 1, 1 );
            fheroes2::Copy( font[64], 4, 3, font[64], 5, 1, 1, 1 );
            fheroes2::Copy( font[64], 8, 6, font[64], 5, 0, 1, 1 );
            fheroes2::Copy( font[64], 8, 6, font[64], 6, 1, 1, 1 );
            fheroes2::Copy( font[64], 4, 8, font[64], 6, 2, 1, 1 );
            font[64].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::updateShadow( font[64], { -1, 2 }, 2 );

            font[91] = font[28];

            font[92].resize( font[69].width(), font[69].height() + 3 );
            font[92].reset();
            fheroes2::Copy( font[69], 0, 0, font[92], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[3], 2, 0, font[92], 3, 0, 5, 2 );
            font[92].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::updateShadow( font[92], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[94], { -1, 2 }, 2 );

            font[95] = font[30];
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::SMALFONT];

            font[3].resize( font[79].width(), font[79].height() + 2 );
            font[3].reset();
            fheroes2::Copy( font[79], 0, 0, font[3], 0, 2, font[79].width(), font[79].height() );
            font[3].setPosition( font[79].x(), font[79].y() - 2 );
            fheroes2::Copy( font[3], 2, 2, font[3], 2, 0, 1, 1 );
            fheroes2::Copy( font[3], 2, 2, font[3], 4, 0, 1, 1 );
            fheroes2::updateShadow( font[3], { -1, 1 }, 2 );

            font[4].resize( font[85].width(), font[85].height() + 2 );
            font[4].reset();
            fheroes2::Copy( font[85], 0, 0, font[4], 0, 2, font[85].width(), font[85].height() );
            font[4].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::Copy( font[4], 1, 2, font[4], 3, 0, 1, 1 );
            fheroes2::Copy( font[4], 1, 2, font[4], 5, 0, 1, 1 );
            fheroes2::updateShadow( font[4], { -1, 1 }, 2 );

            font[6].resize( font[85].width(), font[85].height() + 2 );
            font[6].reset();
            fheroes2::Copy( font[85], 0, 0, font[6], 0, 2, font[85].width(), font[85].height() );
            font[6].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::Copy( font[6], 1, 2, font[6], 4, 0, 1, 1 );
            fheroes2::updateShadow( font[6], { -1, 1 }, 2 );

            font[10].resize( font[65].width(), font[65].height() + 2 );
            font[10].reset();
            fheroes2::Copy( font[65], 0, 0, font[10], 0, 2, font[65].width(), font[65].height() );
            font[10].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::Copy( font[10], 2, 2, font[10], 2, 1, 1, 1 );
            fheroes2::Copy( font[10], 2, 2, font[10], 4, 1, 1, 1 );
            fheroes2::Copy( font[10], 2, 2, font[10], 3, 0, 1, 1 );
            fheroes2::updateShadow( font[10], { -1, 1 }, 2 );

            font[28] = font[73];
            fheroes2::FillTransform( font[28], 0, 0, font[28].width(), 2, 1 );
            fheroes2::Copy( font[28], 1, 2, font[28], 1, 0, 1, 1 );
            fheroes2::Copy( font[28], 1, 2, font[28], 3, 0, 1, 1 );
            fheroes2::updateShadow( font[28], { -1, 1 }, 2 );

            font[30] = font[28];

            font[32].resize( font[65].width(), font[65].height() + 2 );
            font[32].reset();
            fheroes2::Copy( font[65], 0, 0, font[32], 0, 2, font[65].width(), font[65].height() );
            font[32].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::Copy( font[32], 2, 2, font[32], 3, 0, 1, 1 );
            fheroes2::Copy( font[32], 2, 2, font[32], 4, 1, 1, 1 );
            fheroes2::updateShadow( font[32], { -1, 1 }, 2 );

            font[62].resize( font[67].width(), font[67].height() + 2 );
            font[62].reset();
            fheroes2::Copy( font[67], 0, 0, font[62], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[62], 3, 4, font[62], 3, 5, 1, 1 );
            fheroes2::Copy( font[62], 3, 4, font[62], 2, 6, 1, 1 );
            fheroes2::updateShadow( font[62], { -1, 1 }, 2 );

            font[64].resize( font[69].width(), font[69].height() + 2 );
            font[64].reset();
            fheroes2::Copy( font[69], 0, 0, font[64], 0, 2, font[69].width(), font[69].height() );
            font[64].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[64], 2, 2, font[64], 2, 0, 1, 1 );
            fheroes2::Copy( font[64], 2, 2, font[64], 3, 1, 1, 1 );
            fheroes2::updateShadow( font[64], { -1, 1 }, 2 );

            font[91] = font[28];

            font[92].resize( font[69].width(), font[69].height() + 2 );
            font[92].reset();
            fheroes2::Copy( font[69], 0, 0, font[92], 0, 2, font[69].width(), font[69].height() );
            font[92].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[92], 2, 2, font[92], 3, 0, 1, 1 );
            fheroes2::Copy( font[92], 2, 2, font[92], 2, 1, 1, 1 );
            fheroes2::Copy( font[92], 2, 2, font[92], 4, 1, 1, 1 );
            fheroes2::updateShadow( font[92], { -1, 1 }, 2 );

            font[93] = font[28];

            font[94].resize( font[69].width(), font[69].height() + 2 );
            font[94].reset();
            fheroes2::Copy( font[69], 0, 0, font[94], 0, 2, font[69].width(), font[69].height() );
            font[94].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[94], 2, 2, font[94], 4, 0, 1, 1 );
            fheroes2::Copy( font[94], 2, 2, font[94], 3, 1, 1, 1 );
            fheroes2::updateShadow( font[94], { -1, 1 }, 2 );

            font[95] = font[28];
        }
    }

    void generateRussianAlphabet()
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = _icnVsSprite[icnId];

            original.resize( 96 );
            original.insert( original.end(), 128, original[0] );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::FONT];

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
            fheroes2::updateShadow( font[168 - 32], { -1, 2 }, 2 );

            font[192 - 32] = font[33];

            font[193 - 32] = font[34 + offset];
            fheroes2::FillTransform( font[193 - 32], 9, 4, 2, 1, 1 );
            fheroes2::Copy( font[38], 6, 0, font[193 - 32], 6, 0, 5, 4 );
            fheroes2::Copy( font[193 - 32], 9, 5, font[193 - 32], 8, 4, 1, 1 );
            fheroes2::updateShadow( font[193 - 32], { -1, 2 }, 2 );

            font[194 - 32] = font[34 + offset];

            font[195 - 32] = font[38];
            fheroes2::FillTransform( font[195 - 32], 6, 4, 3, 4, 1 );

            font[196 - 32] = font[36 + offset];

            font[197 - 32] = font[37 + offset];

            // x with | in the middle.
            font[198 - 32].resize( font[56].width() + 1, font[56].height() );
            font[198 - 32].reset();
            fheroes2::Copy( font[56], 1, 0, font[198 - 32], 1, 0, 8, 11 );
            fheroes2::Copy( font[56], 9, 0, font[198 - 32], 10, 0, 6, 11 );
            fheroes2::Fill( font[198 - 32], 9, 1, 1, 9, font[198 - 32].image()[1 + font[198 - 32].width()] );
            font[198 - 32].setPosition( font[56].x(), font[56].y() );
            fheroes2::updateShadow( font[198 - 32], { -1, 2 }, 2 );

            font[199 - 32].resize( font[19].width() + 1, font[19].height() );
            font[199 - 32].reset();
            fheroes2::Copy( font[19], 1, 0, font[199 - 32], 1, 0, 5, 3 );
            fheroes2::Copy( font[19], 5, 0, font[199 - 32], 6, 0, 3, 4 );
            fheroes2::Copy( font[19], 3, 5, font[199 - 32], 4, 4, 5, 4 );
            fheroes2::Copy( font[19], 1, 8, font[199 - 32], 1, 8, 5, 3 );
            fheroes2::Copy( font[19], 5, 8, font[199 - 32], 6, 8, 3, 3 );
            fheroes2::FillTransform( font[199 - 32], 2, 6, 5, 3, 1 );
            font[199 - 32].setPosition( font[19].x(), font[19].y() );
            fheroes2::updateShadow( font[199 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[200 - 32], { -1, 2 }, 2 );

            font[201 - 32].resize( font[200 - 32].width(), font[200 - 32].height() + 3 );
            font[201 - 32].reset();
            fheroes2::Copy( font[200 - 32], 0, 0, font[201 - 32], 0, 3, font[200 - 32].width(), font[200 - 32].height() );
            font[201 - 32].setPosition( font[200 - 32].x(), font[200 - 32].y() - 3 );
            fheroes2::Copy( font[201 - 32], 12, 4, font[201 - 32], 8, 0, 1, 1 );
            fheroes2::Copy( font[201 - 32], 11, 10, font[201 - 32], 8, 1, 1, 1 );
            fheroes2::updateShadow( font[201 - 32], { -1, 2 }, 2 );

            font[202 - 32] = font[43 + offset];

            font[204 - 32] = font[45 + offset];
            font[205 - 32] = font[40 + offset];
            font[206 - 32] = font[47 + offset];

            font[207 - 32] = font[195 - 32];
            fheroes2::Copy( font[207 - 32], 4, 1, font[207 - 32], 8, 1, 2, 9 );
            fheroes2::Copy( font[207 - 32], 4, 9, font[207 - 32], 8, 10, 2, 1 );
            fheroes2::Copy( font[207 - 32], 6, 0, font[207 - 32], 10, 0, 1, 2 );
            fheroes2::updateShadow( font[207 - 32], { -1, 2 }, 2 );

            font[203 - 32].resize( font[207 - 32].width() - 1, font[207 - 32].height() );
            font[203 - 32].reset();
            fheroes2::Copy( font[207 - 32], 0, 0, font[203 - 32], 0, 0, font[207 - 32].width() - 1, font[207 - 32].height() );
            fheroes2::FillTransform( font[203 - 32], 0, 0, 4, 6, 1 );
            fheroes2::FillTransform( font[203 - 32], 4, 0, 3, 2, 1 );
            fheroes2::Copy( font[203 - 32], 4, 2, font[203 - 32], 5, 1, 2, 1 );
            fheroes2::Copy( font[203 - 32], 1, 10, font[203 - 32], 5, 0, 2, 1 );
            font[203 - 32].setPosition( font[207 - 32].x(), font[207 - 32].y() );
            fheroes2::updateShadow( font[203 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[212 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[214 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[216 - 32], { -1, 2 }, 2 );

            font[215 - 32] = font[53];
            fheroes2::FillTransform( font[215 - 32], 3, 6, 6, 7, 1 );
            fheroes2::Copy( font[216 - 32], 4, 5, font[215 - 32], 4, 3, 4, 6 );
            fheroes2::Copy( font[215 - 32], 6, 5, font[215 - 32], 8, 3, 1, 3 );
            fheroes2::Copy( font[215 - 32], 7, 4, font[215 - 32], 9, 2, 1, 2 );
            fheroes2::Copy( font[215 - 32], 9, 8, font[215 - 32], 9, 9, 1, 1 );
            fheroes2::updateShadow( font[215 - 32], { -1, 2 }, 2 );

            font[217 - 32].resize( font[216 - 32].width() + 2, font[216 - 32].height() + 1 );
            font[217 - 32].reset();
            fheroes2::Copy( font[216 - 32], 0, 0, font[217 - 32], 0, 0, font[216 - 32].width(), font[216 - 32].height() );
            fheroes2::Copy( font[214 - 32], 11, 8, font[217 - 32], 14, 8, 3, 4 );
            font[217 - 32].setPosition( font[216 - 32].x(), font[216 - 32].y() );
            fheroes2::updateShadow( font[217 - 32], { -1, 2 }, 2 );

            font[218 - 32].resize( font[193 - 32].width() + 1, font[193 - 32].height() );
            font[218 - 32].reset();
            fheroes2::Copy( font[193 - 32], 0, 0, font[218 - 32], 1, 0, font[193 - 32].width(), font[193 - 32].height() );
            fheroes2::Copy( font[193 - 32], 1, 0, font[218 - 32], 1, 0, 3, 4 );
            fheroes2::FillTransform( font[218 - 32], 7, 0, 5, 4, 1 );
            font[218 - 32].setPosition( font[193 - 32].x(), font[193 - 32].y() );
            fheroes2::updateShadow( font[218 - 32], { -1, 2 }, 2 );

            font[220 - 32] = font[193 - 32];
            fheroes2::FillTransform( font[220 - 32], 0, 0, 4, 6, 1 );
            fheroes2::FillTransform( font[220 - 32], 6, 0, 5, 4, 1 );
            fheroes2::Copy( font[53], 8, 0, font[220 - 32], 3, 0, 3, 1 );
            fheroes2::updateShadow( font[220 - 32], { -1, 2 }, 2 );

            font[219 - 32].resize( font[220 - 32].width() + 3, font[220 - 32].height() );
            font[219 - 32].reset();
            fheroes2::Copy( font[220 - 32], 0, 0, font[219 - 32], 0, 0, font[220 - 32].width(), font[220 - 32].height() );
            fheroes2::Copy( font[219 - 32], 3, 0, font[219 - 32], 11, 0, 3, 9 );
            fheroes2::Copy( font[207 - 32], 8, 9, font[219 - 32], 12, 9, 2, 2 );
            font[219 - 32].setPosition( font[220 - 32].x(), font[220 - 32].y() );
            fheroes2::updateShadow( font[219 - 32], { -1, 2 }, 2 );

            font[221 - 32].resize( font[47].width() - 3, font[47].height() );
            font[221 - 32].reset();
            fheroes2::Copy( font[47], 4, 0, font[221 - 32], 1, 0, 9, 11 );
            fheroes2::FillTransform( font[221 - 32], 0, 3, 3, 5, 1 );
            fheroes2::Copy( font[221 - 32], 3, 0, font[221 - 32], 4, 5, 5, 1 );
            font[221 - 32].setPosition( font[47].x(), font[47].y() );
            fheroes2::updateShadow( font[221 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[222 - 32], { -1, 2 }, 2 );

            font[223 - 32].resize( font[203 - 32].width() - 1, font[203 - 32].height() );
            font[223 - 32].reset();
            fheroes2::Copy( font[33], 0, 5, font[223 - 32], 0, 5, 7, 6 );
            fheroes2::Copy( font[212 - 32], 0, 0, font[223 - 32], 1, 0, 7, 6 );
            fheroes2::Copy( font[203 - 32], 8, 0, font[223 - 32], 7, 0, 2, 11 );
            fheroes2::Copy( font[223 - 32], 6, 5, font[223 - 32], 7, 5, 1, 1 );
            font[223 - 32].setPosition( font[203 - 32].x(), font[203 - 32].y() );
            fheroes2::updateShadow( font[223 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[225 - 32], { -1, 2 }, 2 );

            font[227 - 32] = font[82];
            fheroes2::Copy( font[227 - 32], 1, 0, font[227 - 32], 3, 0, 2, 1 );
            fheroes2::Copy( font[227 - 32], 4, 2, font[227 - 32], 4, 1, 1, 1 );
            fheroes2::SetTransformPixel( font[227 - 32], 4, 2, 1 );
            fheroes2::updateShadow( font[227 - 32], { -1, 2 }, 2 );

            font[228 - 32] = font[71];
            font[229 - 32] = font[37 + offset];

            // x with | in the middle.
            font[230 - 32].resize( font[88].width() + 2, font[88].height() );
            font[230 - 32].reset();
            fheroes2::Copy( font[88], 0, 0, font[230 - 32], 0, 0, 6, 7 );
            fheroes2::Copy( font[88], 5, 0, font[230 - 32], 7, 0, 5, 7 );
            fheroes2::Fill( font[230 - 32], 6, 1, 1, 5, font[230 - 32].image()[3 + font[230 - 32].width()] );
            font[230 - 32].setPosition( font[88].x(), font[88].y() );
            fheroes2::updateShadow( font[230 - 32], { -1, 2 }, 2 );

            // letter 3 (z)
            font[231 - 32].resize( font[19].width(), font[19].height() - 4 );
            font[231 - 32].reset();
            fheroes2::Copy( font[19], 0, 0, font[231 - 32], 0, 0, font[19].width(), 3 );
            fheroes2::Copy( font[19], 0, 5, font[231 - 32], 0, 3, font[19].width(), 1 );
            fheroes2::Copy( font[19], 0, 8, font[231 - 32], 0, 4, font[19].width(), 4 );
            fheroes2::FillTransform( font[231 - 32], 0, 2, 3, 3, 1 );
            font[231 - 32].setPosition( font[19].x(), font[19].y() + 4 );
            fheroes2::updateShadow( font[231 - 32], { -1, 2 }, 2 );

            // letter B (v)
            font[226 - 32].resize( font[231 - 32].width() + 1, font[231 - 32].height() );
            font[226 - 32].reset();
            fheroes2::Copy( font[231 - 32], 0, 0, font[226 - 32], 1, 0, font[231 - 32].width(), font[231 - 32].height() );
            fheroes2::Copy( font[77], 1, 0, font[226 - 32], 1, 0, 3, 7 );
            fheroes2::Copy( font[226 - 32], 7, 1, font[226 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[226 - 32], 7, 1, font[226 - 32], 3, 6, 1, 1 );
            fheroes2::Copy( font[226 - 32], 3, 4, font[226 - 32], 3, 5, 1, 1 );
            font[226 - 32].setPosition( font[231 - 32].x(), font[231 - 32].y() );
            fheroes2::updateShadow( font[226 - 32], { -1, 2 }, 2 );

            font[232 - 32] = font[85];

            font[233 - 32].resize( font[232 - 32].width(), font[232 - 32].height() + 3 );
            font[233 - 32].reset();
            fheroes2::Copy( font[232 - 32], 0, 0, font[233 - 32], 0, 3, font[232 - 32].width(), font[232 - 32].height() );
            fheroes2::Copy( font[233 - 32], 8, 3, font[233 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[233 - 32], 7, 3, font[233 - 32], 5, 1, 1, 1 );
            font[233 - 32].setPosition( font[232 - 32].x(), font[232 - 32].y() - 3 );
            fheroes2::updateShadow( font[233 - 32], { -1, 2 }, 2 );

            // Shorter k.
            font[234 - 32].resize( font[75].width() - 1, font[75].height() - 4 );
            font[234 - 32].reset();
            fheroes2::Copy( font[75], 2, 2, font[234 - 32], 2, 0, 7, 4 );
            fheroes2::Copy( font[75], 1, 0, font[234 - 32], 1, 0, 3, 1 );
            fheroes2::Copy( font[75], 0, 7, font[234 - 32], 0, 4, font[75].width(), 2 );
            fheroes2::Copy( font[75], 0, 10, font[234 - 32], 0, 6, 4, 1 );
            fheroes2::Copy( font[75], 7, 10, font[234 - 32], 6, 6, 3, 1 );
            font[234 - 32].setPosition( font[75].x(), font[75].y() + 4 );
            fheroes2::updateShadow( font[234 - 32], { -1, 2 }, 2 );

            font[235 - 32] = font[78];
            fheroes2::Copy( font[235 - 32], 3, 0, font[235 - 32], 2, 1, 1, 1 );
            fheroes2::FillTransform( font[235 - 32], 0, 0, 2, 3, 1 );
            fheroes2::FillTransform( font[235 - 32], 2, 0, 1, 1, 1 );
            fheroes2::updateShadow( font[235 - 32], { -1, 2 }, 2 );

            font[236 - 32] = font[45 + offset];
            fheroes2::Copy( font[87], 9, 0, font[236 - 32], 3, 0, 4, 7 );
            fheroes2::Copy( font[87], 9, 0, font[236 - 32], 9, 0, 4, 7 );
            fheroes2::FillTransform( font[236 - 32], 0, 0, 3, 6, 1 );
            fheroes2::updateShadow( font[236 - 32], { -1, 2 }, 2 );

            font[237 - 32] = font[78];
            fheroes2::FillTransform( font[237 - 32], 4, 0, 3, 8, 1 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 4, 3, 1, 2 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 5, 3, 1, 2 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 6, 3, 1, 2 );
            fheroes2::Copy( font[78], 4, 1, font[237 - 32], 7, 3, 1, 1 );
            fheroes2::updateShadow( font[237 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[244 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[246 - 32], { -1, 2 }, 2 );

            font[247 - 32] = font[85];
            fheroes2::Copy( font[247 - 32], 2, 5, font[247 - 32], 2, 3, 6, 2 );
            fheroes2::Copy( font[247 - 32], 8, 0, font[247 - 32], 7, 4, 1, 1 );
            fheroes2::Copy( font[247 - 32], 8, 0, font[247 - 32], 7, 5, 1, 1 );
            fheroes2::Copy( font[247 - 32], 8, 0, font[247 - 32], 7, 6, 1, 1 );
            fheroes2::FillTransform( font[247 - 32], 1, 5, 6, 4, 1 );
            fheroes2::updateShadow( font[247 - 32], { -1, 2 }, 2 );

            font[248 - 32].resize( font[85].width() + 3, font[85].height() );
            font[248 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[248 - 32], 0, 0, 4, 7 );
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 5, 0, 4, 7 );
            fheroes2::Copy( font[85], 6, 0, font[248 - 32], 9, 0, 4, 7 );
            fheroes2::Copy( font[248 - 32], 8, 5, font[248 - 32], 4, 5, 4, 2 );
            font[248 - 32].setPosition( font[85].x(), font[85].y() );
            fheroes2::updateShadow( font[248 - 32], { -1, 2 }, 2 );

            font[249 - 32].resize( font[248 - 32].width() + 2, font[248 - 32].height() );
            font[249 - 32].reset();
            fheroes2::Copy( font[248 - 32], 0, 0, font[249 - 32], 0, 0, 12, 7 );
            fheroes2::Copy( font[246 - 32], 9, 4, font[249 - 32], 12, 4, 3, 4 );
            font[249 - 32].setPosition( font[248 - 32].x(), font[248 - 32].y() );
            fheroes2::updateShadow( font[249 - 32], { -1, 2 }, 2 );

            font[252 - 32] = font[226 - 32];
            fheroes2::FillTransform( font[252 - 32], 4, 0, 5, 3, 1 );

            font[250 - 32].resize( font[252 - 32].width() + 1, font[252 - 32].height() );
            font[250 - 32].reset();
            fheroes2::Copy( font[252 - 32], 0, 0, font[250 - 32], 1, 0, font[252 - 32].width(), font[252 - 32].height() );
            fheroes2::Copy( font[252 - 32], 1, 0, font[250 - 32], 1, 0, 1, 2 );
            font[250 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            fheroes2::updateShadow( font[250 - 32], { -1, 2 }, 2 );

            font[251 - 32].resize( font[252 - 32].width() + 3, font[252 - 32].height() );
            font[251 - 32].reset();
            fheroes2::Copy( font[252 - 32], 0, 0, font[251 - 32], 0, 0, font[252 - 32].width(), font[252 - 32].height() );
            fheroes2::Copy( font[252 - 32], 2, 0, font[251 - 32], 10, 0, 2, 7 );
            font[251 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            fheroes2::updateShadow( font[251 - 32], { -1, 2 }, 2 );

            font[253 - 32] = font[79];
            fheroes2::FillTransform( font[253 - 32], 0, 2, 3, 3, 1 );
            fheroes2::Copy( font[253 - 32], 8, 3, font[253 - 32], 7, 3, 1, 1 );
            fheroes2::Copy( font[253 - 32], 8, 3, font[253 - 32], 6, 3, 1, 1 );
            fheroes2::Copy( font[253 - 32], 8, 3, font[253 - 32], 5, 3, 1, 1 );
            fheroes2::updateShadow( font[253 - 32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[254 - 32], { -1, 2 }, 2 );

            font[255 - 32] = font[65];
            fheroes2::FillTransform( font[255 - 32], 0, 2, 6, 3, 1 );
            fheroes2::Copy( font[69], 2, 5, font[255 - 32], 1, 2, 6, 2 );
            fheroes2::Copy( font[255 - 32], 6, 4, font[255 - 32], 6, 3, 1, 1 );
            fheroes2::updateShadow( font[255 - 32], { -1, 2 }, 2 );
        }

        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::SMALFONT];

            size_t offset = 0;

            // E with 2 dots on top.
            font[168 - 32].resize( font[37].width(), font[37].height() + 2 );
            font[168 - 32].reset();
            fheroes2::Copy( font[37], 0, 0, font[168 - 32], 0, 2, font[37].width(), font[37].height() );
            fheroes2::Copy( font[37], 3, 0, font[168 - 32], 3, 0, 1, 1 );
            fheroes2::Copy( font[37], 3, 0, font[168 - 32], 5, 0, 1, 1 );
            font[168 - 32].setPosition( font[37].x(), font[37].y() );
            fheroes2::updateShadow( font[168 - 32], { -1, 1 }, 2 );

            font[192 - 32] = font[33 + offset];

            font[193 - 32] = font[34 + offset];
            fheroes2::FillTransform( font[193 - 32], 5, 1, 2, 2, 1 );
            fheroes2::Copy( font[193 - 32], 5, 0, font[193 - 32], 6, 0, 1, 1 );
            fheroes2::updateShadow( font[193 - 32], { -1, 1 }, 2 );

            font[194 - 32] = font[34 + offset];

            font[195 - 32].resize( font[193 - 32].width() + 1, font[193 - 32].height() );
            font[195 - 32].reset();
            fheroes2::Copy( font[193 - 32], 0, 0, font[195 - 32], 0, 0, 4, 8 );
            fheroes2::Copy( font[193 - 32], 3, 0, font[195 - 32], 4, 0, 4, 1 );
            font[195 - 32].setPosition( font[193 - 32].x(), font[193 - 32].y() );
            fheroes2::updateShadow( font[195 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[198 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[199 - 32], { -1, 1 }, 2 );

            font[200 - 32] = font[40];
            fheroes2::FillTransform( font[200 - 32], 4, 2, 3, 4, 1 );
            fheroes2::Copy( font[40], 3, 0, font[200 - 32], 4, 4, 1, 1 );
            fheroes2::Copy( font[40], 3, 0, font[200 - 32], 5, 3, 1, 1 );
            fheroes2::Copy( font[40], 3, 0, font[200 - 32], 6, 2, 1, 1 );
            fheroes2::updateShadow( font[200 - 32], { -1, 1 }, 2 );

            font[201 - 32].resize( font[200 - 32].width(), font[200 - 32].height() + 2 );
            font[201 - 32].reset();
            fheroes2::Copy( font[200 - 32], 1, 0, font[201 - 32], 1, 2, 8, 7 );
            fheroes2::Copy( font[200 - 32], 2, 0, font[201 - 32], 5, 0, 2, 1 );
            font[201 - 32].setPosition( font[200 - 32].x(), font[200 - 32].y() - 2 );
            fheroes2::updateShadow( font[201 - 32], { -1, 1 }, 2 );

            font[202 - 32] = font[43 + offset];

            font[203 - 32].resize( font[34].width(), font[34].height() );
            font[203 - 32].reset();
            fheroes2::Copy( font[34], 1, 0, font[203 - 32], 1, 0, 3, 7 );
            fheroes2::Copy( font[34], 3, 0, font[203 - 32], 6, 0, 1, 7 );
            fheroes2::Copy( font[34], 3, 0, font[203 - 32], 4, 0, 2, 1 );
            fheroes2::FillTransform( font[203 - 32], 1, 0, 2, 2, 1 );
            fheroes2::FillTransform( font[203 - 32], 3, 0, 1, 1, 1 );
            font[203 - 32].setPosition( font[34].x(), font[34].y() );
            fheroes2::updateShadow( font[203 - 32], { -1, 1 }, 2 );

            font[204 - 32] = font[45 + offset];
            font[205 - 32] = font[40 + offset];
            font[206 - 32] = font[47 + offset];

            font[207 - 32] = font[195 - 32];
            fheroes2::Copy( font[207 - 32], 3, 0, font[207 - 32], 6, 0, 1, 7 );
            fheroes2::updateShadow( font[207 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[214 - 32], { -1, 1 }, 2 );

            font[215 - 32] = font[53];
            fheroes2::Copy( font[53], 3, 5, font[215 - 32], 3, 2, 4, 2 );
            fheroes2::FillTransform( font[215 - 32], 2, 4, 5, 4, 1 );
            fheroes2::updateShadow( font[215 - 32], { -1, 1 }, 2 );

            font[216 - 32].resize( font[53].width(), font[53].height() );
            font[216 - 32].reset();
            fheroes2::Copy( font[53], 1, 0, font[216 - 32], 1, 0, 4, 7 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 6, 1, 2, 6 );
            fheroes2::Copy( font[44], 3, 0, font[216 - 32], 9, 0, 1, 8 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 5, 5, 1, 1 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 8, 5, 1, 1 );
            fheroes2::Copy( font[53], 7, 1, font[216 - 32], 6, 0, 1, 1 );
            font[216 - 32].setPosition( font[53].x(), font[53].y() );
            fheroes2::updateShadow( font[216 - 32], { -1, 1 }, 2 );

            font[217 - 32].resize( font[216 - 32].width() + 2, font[216 - 32].height() + 1 );
            font[217 - 32].reset();
            fheroes2::Copy( font[216 - 32], 1, 0, font[217 - 32], 1, 0, 9, 7 );
            fheroes2::Copy( font[216 - 32], 3, 0, font[217 - 32], 10, 6, 1, 1 );
            fheroes2::Copy( font[216 - 32], 3, 0, font[217 - 32], 11, 5, 1, 3 );
            font[217 - 32].setPosition( font[216 - 32].x(), font[216 - 32].y() );
            fheroes2::updateShadow( font[217 - 32], { -1, 1 }, 2 );

            font[220 - 32].resize( font[34].width(), font[34].height() );
            font[220 - 32].reset();
            fheroes2::Copy( font[34], 2, 0, font[220 - 32], 1, 0, 2, 7 );
            fheroes2::Copy( font[34], 4, 3, font[220 - 32], 3, 3, 1, 4 );
            fheroes2::Copy( font[34], 4, 3, font[220 - 32], 4, 3, 3, 4 );
            fheroes2::FillTransform( font[220 - 32], 1, 0, 1, 1, 1 );
            font[220 - 32].setPosition( font[34].x(), font[34].y() );
            fheroes2::updateShadow( font[220 - 32], { -1, 1 }, 2 );

            font[219 - 32].resize( font[220 - 32].width() + 2, font[220 - 32].height() );
            font[219 - 32].reset();
            fheroes2::Copy( font[220 - 32], 1, 0, font[219 - 32], 1, 0, 6, 7 );
            fheroes2::Copy( font[220 - 32], 2, 0, font[219 - 32], 8, 0, 1, 7 );
            font[219 - 32].setPosition( font[220 - 32].x(), font[220 - 32].y() );
            fheroes2::updateShadow( font[219 - 32], { -1, 1 }, 2 );

            font[218 - 32].resize( font[220 - 32].width() + 2, font[220 - 32].height() );
            font[218 - 32].reset();
            fheroes2::Copy( font[220 - 32], 1, 0, font[218 - 32], 3, 0, 6, 7 );
            fheroes2::Copy( font[220 - 32], 2, 3, font[218 - 32], 1, 0, 3, 1 );
            fheroes2::Copy( font[220 - 32], 2, 3, font[218 - 32], 1, 1, 1, 1 );
            font[218 - 32].setPosition( font[220 - 32].x(), font[220 - 32].y() );
            fheroes2::updateShadow( font[218 - 32], { -1, 1 }, 2 );

            font[221 - 32].resize( font[47].width() - 1, font[47].height() );
            font[221 - 32].reset();
            fheroes2::Copy( font[47], 2, 0, font[221 - 32], 1, 0, 6, 7 );
            fheroes2::Copy( font[47], 3, 0, font[221 - 32], 4, 3, 2, 1 );
            font[221 - 32].setPosition( font[47].x(), font[47].y() );
            fheroes2::updateShadow( font[221 - 32], { -1, 1 }, 2 );

            font[222 - 32].resize( font[47].width() + 1, font[47].height() );
            font[222 - 32].reset();
            fheroes2::Copy( font[44], 2, 0, font[222 - 32], 1, 0, 2, 7 );
            fheroes2::Copy( font[47], 2, 0, font[222 - 32], 4, 0, 5, 2 );
            fheroes2::Copy( font[47], 2, 5, font[222 - 32], 4, 5, 5, 2 );
            fheroes2::Copy( font[222 - 32], 1, 0, font[222 - 32], 3, 3, 1, 1 );
            fheroes2::Copy( font[222 - 32], 2, 0, font[222 - 32], 4, 2, 1, 3 );
            fheroes2::Copy( font[222 - 32], 2, 0, font[222 - 32], 8, 2, 1, 3 );
            font[222 - 32].setPosition( font[47].x(), font[47].y() );
            fheroes2::updateShadow( font[222 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[223 - 32], { -1, 1 }, 2 );

            offset = 32;

            // e with 2 dots on top.
            font[184 - 32].resize( font[69].width(), font[69].height() + 2 );
            font[184 - 32].reset();
            fheroes2::Copy( font[69], 0, 0, font[184 - 32], 0, 2, font[69].width(), font[69].height() );
            fheroes2::Copy( font[69], 2, 0, font[184 - 32], 2, 0, 1, 1 );
            fheroes2::Copy( font[69], 2, 0, font[184 - 32], 4, 0, 1, 1 );
            font[184 - 32].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::updateShadow( font[184 - 32], { -1, 1 }, 2 );

            font[224 - 32] = font[33 + offset];

            font[225 - 32].resize( font[34].width(), font[34].height() );
            font[225 - 32].reset();
            fheroes2::Copy( font[34], 4, 3, font[225 - 32], 4, 3, 3, 4 );
            fheroes2::Copy( font[65], 1, 2, font[225 - 32], 2, 4, 2, 3 );
            fheroes2::FillTransform( font[225 - 32], 3, 5, 1, 1, 1 );
            fheroes2::Copy( font[225 - 32], 2, 5, font[225 - 32], 2, 1, 2, 2 );
            fheroes2::Copy( font[37], 2, 0, font[225 - 32], 2, 0, 5, 1 );
            font[225 - 32].setPosition( font[34].x(), font[34].y() );
            fheroes2::updateShadow( font[225 - 32], { -1, 1 }, 2 );

            font[226 - 32].resize( font[82].width() - 1, font[82].height() );
            font[226 - 32].reset();
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 3, 0, 2, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 3, 2, 2, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 3, 4, 2, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 5, 1, 1, 1 );
            fheroes2::Copy( font[82], 1, 0, font[226 - 32], 5, 3, 1, 1 );
            font[226 - 32].setPosition( font[82].x(), font[82].y() );
            fheroes2::updateShadow( font[226 - 32], { -1, 1 }, 2 );

            font[227 - 32] = font[82];
            fheroes2::Copy( font[227 - 32], 3, 1, font[227 - 32], 3, 0, 1, 1 );
            fheroes2::FillTransform( font[227 - 32], 3, 1, 1, 1, 1 );
            fheroes2::updateShadow( font[227 - 32], { -1, 1 }, 2 );

            font[228 - 32] = font[71];

            font[229 - 32] = font[37 + offset];

            font[230 - 32].resize( font[88].width() + 1, font[88].height() );
            font[230 - 32].reset();
            fheroes2::Copy( font[88], 0, 0, font[230 - 32], 0, 0, 4, 5 );
            fheroes2::Copy( font[88], 4, 0, font[230 - 32], 5, 0, 4, 5 );
            fheroes2::Copy( font[85], 2, 0, font[230 - 32], 4, 0, 1, 4 );
            fheroes2::Copy( font[85], 2, 0, font[230 - 32], 4, 4, 1, 1 );
            font[230 - 32].setPosition( font[88].x(), font[88].y() );
            fheroes2::updateShadow( font[230 - 32], { -1, 1 }, 2 );

            font[232 - 32] = font[85];

            font[233 - 32].resize( font[232 - 32].width(), font[232 - 32].height() + 2 );
            font[233 - 32].reset();
            fheroes2::Copy( font[232 - 32], 1, 0, font[233 - 32], 1, 2, 7, 5 );
            fheroes2::Copy( font[232 - 32], 1, 0, font[233 - 32], 3, 0, 2, 1 );
            font[233 - 32].setPosition( font[232 - 32].x(), font[232 - 32].y() - 2 );
            fheroes2::updateShadow( font[233 - 32], { -1, 1 }, 2 );

            font[234 - 32].resize( font[75].width() - 2, font[75].height() - 2 );
            font[234 - 32].reset();
            fheroes2::Copy( font[75], 1, 0, font[234 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[75], 4, 1, font[234 - 32], 3, 0, 3, 3 );
            fheroes2::Copy( font[75], 5, 4, font[234 - 32], 4, 3, 2, 2 );
            font[234 - 32].setPosition( font[75].x(), font[75].y() + 2 );
            fheroes2::updateShadow( font[234 - 32], { -1, 1 }, 2 );

            font[235 - 32].resize( font[65].width() - 2, font[65].height() );
            font[235 - 32].reset();
            fheroes2::Copy( font[203 - 32], 2, 3, font[235 - 32], 1, 1, 2, 4 );
            fheroes2::Copy( font[203 - 32], 5, 0, font[235 - 32], 3, 0, 2, 5 );
            fheroes2::FillTransform( font[235 - 32], 1, 1, 1, 1, 1 );
            font[235 - 32].setPosition( font[65].x(), font[65].y() );
            fheroes2::updateShadow( font[235 - 32], { -1, 1 }, 2 );

            font[236 - 32].resize( font[235 - 32].width() + 3, font[235 - 32].height() );
            font[236 - 32].reset();
            fheroes2::Copy( font[235 - 32], 4, 0, font[236 - 32], 4, 0, 1, 5 );
            fheroes2::Copy( font[235 - 32], 1, 0, font[236 - 32], 1, 1, 3, 3 );
            fheroes2::Copy( font[236 - 32], 2, 0, font[236 - 32], 5, 0, 3, 5 );
            fheroes2::Copy( font[236 - 32], 4, 0, font[236 - 32], 1, 4, 1, 1 );
            font[236 - 32].setPosition( font[235 - 32].x(), font[235 - 32].y() );
            fheroes2::updateShadow( font[236 - 32], { -1, 1 }, 2 );

            font[237 - 32].resize( font[72].width() - 2, font[72].height() - 2 );
            font[237 - 32].reset();
            fheroes2::Copy( font[72], 1, 0, font[237 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[72], 2, 0, font[237 - 32], 5, 0, 1, 5 );
            fheroes2::Copy( font[72], 3, 2, font[237 - 32], 3, 2, 2, 1 );
            font[237 - 32].setPosition( font[72].x(), font[72].y() + 2 );
            fheroes2::updateShadow( font[237 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[242 - 32], { -1, 1 }, 2 );

            font[243 - 32] = font[57 + offset];

            font[244 - 32].resize( font[81].width() + 3, font[81].height() + 1 );
            font[244 - 32].reset();
            fheroes2::Copy( font[81], 1, 0, font[244 - 32], 1, 0, 5, 7 );
            fheroes2::Copy( font[81], 2, 0, font[244 - 32], 6, 0, 4, 4 );
            fheroes2::Copy( font[81], 2, 4, font[244 - 32], 6, 4, 3, 1 );
            fheroes2::Copy( font[81], 2, 4, font[244 - 32], 5, 7, 1, 1 );
            font[244 - 32].setPosition( font[81].x(), font[81].y() );
            fheroes2::updateShadow( font[244 - 32], { -1, 1 }, 2 );

            // Bigger letter
            font[212 - 32] = font[244 - 32];
            fheroes2::Copy( font[212 - 32], 5, 1, font[212 - 32], 5, 0, 1, 1 );
            fheroes2::Copy( font[212 - 32], 5, 1, font[212 - 32], 4, 7, 1, 1 );
            font[212 - 32].setPosition( font[48].x(), font[48].y() ); // copy from a big better
            fheroes2::updateShadow( font[212 - 32], { -1, 1 }, 2 );

            font[245 - 32] = font[56 + offset];

            font[246 - 32].resize( font[85].width() + 1, font[85].height() + 1 );
            font[246 - 32].reset();
            fheroes2::Copy( font[85], 0, 0, font[246 - 32], 0, 0, font[85].width(), font[85].height() );
            fheroes2::Copy( font[246 - 32], 2, 0, font[246 - 32], 8, 3, 1, 3 );
            font[246 - 32].setPosition( font[85].x(), font[85].y() );
            fheroes2::updateShadow( font[246 - 32], { -1, 1 }, 2 );

            font[247 - 32] = font[85];
            fheroes2::Copy( font[85], 2, 4, font[247 - 32], 2, 2, 4, 1 );
            fheroes2::Copy( font[85], 1, 0, font[247 - 32], 6, 4, 1, 1 );
            fheroes2::FillTransform( font[247 - 32], 1, 3, 5, 3, 1 );
            fheroes2::updateShadow( font[247 - 32], { -1, 1 }, 2 );

            font[248 - 32].resize( font[85].width() + 2, font[85].height() );
            font[248 - 32].reset();
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 1, 0, 3, 5 );
            fheroes2::Copy( font[85], 6, 0, font[248 - 32], 5, 0, 2, 5 );
            fheroes2::Copy( font[85], 6, 0, font[248 - 32], 8, 0, 2, 5 );
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 4, 4, 1, 1 );
            fheroes2::Copy( font[85], 1, 0, font[248 - 32], 7, 4, 1, 1 );
            font[248 - 32].setPosition( font[85].x(), font[85].y() );
            fheroes2::updateShadow( font[248 - 32], { -1, 1 }, 2 );

            font[249 - 32].resize( font[248 - 32].width() + 1, font[248 - 32].height() );
            font[249 - 32].reset();
            fheroes2::Copy( font[248 - 32], 1, 0, font[249 - 32], 1, 0, 9, 5 );
            fheroes2::Copy( font[248 - 32], 2, 0, font[249 - 32], 10, 3, 1, 3 );
            font[249 - 32].setPosition( font[248 - 32].x(), font[248 - 32].y() );
            fheroes2::updateShadow( font[249 - 32], { -1, 1 }, 2 );

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
            fheroes2::updateShadow( font[250 - 32], { -1, 1 }, 2 );

            font[251 - 32].resize( font[252 - 32].width() + 2, font[252 - 32].height() );
            font[251 - 32].reset();
            fheroes2::Copy( font[252 - 32], 1, 0, font[251 - 32], 1, 0, 5, 5 );
            fheroes2::Copy( font[252 - 32], 2, 0, font[251 - 32], 7, 0, 1, 5 );
            font[251 - 32].setPosition( font[252 - 32].x(), font[252 - 32].y() );
            fheroes2::updateShadow( font[251 - 32], { -1, 1 }, 2 );

            font[253 - 32].resize( font[79].width() - 1, font[79].height() );
            font[253 - 32].reset();
            fheroes2::Copy( font[79], 2, 0, font[253 - 32], 1, 0, 4, 5 );
            fheroes2::Copy( font[79], 2, 0, font[253 - 32], 2, 2, 2, 1 );
            font[253 - 32].setPosition( font[79].x(), font[79].y() );
            fheroes2::updateShadow( font[253 - 32], { -1, 1 }, 2 );

            font[231 - 32] = font[253 - 32];
            fheroes2::FillTransform( font[231 - 32], 0, 1, 3, 3, 1 );
            fheroes2::FillTransform( font[231 - 32], 4, 2, 1, 1, 1 );
            fheroes2::FillTransform( font[231 - 32], 1, 0, 1, 1, 1 );
            fheroes2::Copy( font[253 - 32], 1, 0, font[231 - 32], 1, 1, 1, 1 );
            fheroes2::updateShadow( font[231 - 32], { -1, 1 }, 2 );

            font[254 - 32].resize( font[79].width() + 2, font[79].height() );
            font[254 - 32].reset();
            fheroes2::Copy( font[82], 1, 0, font[254 - 32], 1, 0, 2, 5 );
            fheroes2::Copy( font[79], 1, 0, font[254 - 32], 4, 0, 3, 5 );
            fheroes2::Copy( font[79], 5, 1, font[254 - 32], 7, 1, 1, 3 );
            fheroes2::Copy( font[79], 5, 1, font[254 - 32], 3, 2, 1, 1 );
            font[254 - 32].setPosition( font[79].x(), font[79].y() );
            fheroes2::updateShadow( font[254 - 32], { -1, 1 }, 2 );

            font[255 - 32].resize( font[65].width() - 1, font[65].height() );
            font[255 - 32].reset();
            fheroes2::Copy( font[65], 1, 2, font[255 - 32], 2, 0, 3, 3 );
            fheroes2::Copy( font[235 - 32], 4, 0, font[255 - 32], 5, 0, 1, 5 );
            fheroes2::Copy( font[33], 1, 5, font[255 - 32], 1, 3, 3, 2 );
            font[255 - 32].setPosition( font[65].x(), font[65].y() );
            fheroes2::updateShadow( font[255 - 32], { -1, 1 }, 2 );
        }
    }

    void generateItalianAlphabet()
    {
        // Resize fonts.
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            _icnVsSprite[icnId].resize( 96 );
        }

        // Normal font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::FONT];

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
            fheroes2::updateShadow( font[3], { -1, 2 }, 2 );

            font[4].resize( font[85].width(), font[85].height() + 3 );
            font[4].reset();
            fheroes2::Copy( font[85], 0, 0, font[4], 0, 3, font[85].width(), font[85].height() );
            fheroes2::Copy( font[3], 2, 0, font[4], 3, 0, 5, 2 );
            font[4].setPosition( font[85].x(), font[85].y() - 3 );
            fheroes2::updateShadow( font[4], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[6], { -1, 2 }, 2 );

            font[10].resize( font[65].width(), font[65].height() + 3 );
            font[10].reset();
            fheroes2::Copy( font[65], 0, 0, font[10], 0, 3, font[65].width(), font[65].height() );
            fheroes2::Copy( font[3], 2, 0, font[10], 2, 0, 5, 2 );
            font[10].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::updateShadow( font[10], { -1, 2 }, 2 );

            font[28] = font[73];
            // Just to be safe and not to write something out of buffer.
            if ( font[28].width() > 2 && font[28].height() > 1 ) {
                font[28].image()[2] = 0;
                font[28].transform()[2] = 1;
                font[28].image()[2 + font[28].width()] = 0;
                font[28].transform()[2 + font[28].width()] = 1;
            }
            fheroes2::Copy( font[28], 3, 0, font[28], 1, 0, 1, 2 );
            fheroes2::updateShadow( font[28], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[30], { -1, 2 }, 2 );

            font[32].resize( font[65].width(), font[65].height() + 3 );
            font[32].reset();
            fheroes2::Copy( font[65], 0, 0, font[32], 0, 3, font[65].width(), font[65].height() );
            font[32].setPosition( font[65].x(), font[65].y() - 3 );
            fheroes2::Copy( font[6], 4, 0, font[32], 3, 0, 4, 2 );
            fheroes2::updateShadow( font[32], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[62], { -1, 2 }, 2 );

            font[64].resize( font[69].width(), font[69].height() + 3 );
            font[64].reset();
            fheroes2::Copy( font[69], 0, 0, font[64], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[64], 4, 3, font[64], 4, 0, 1, 1 );
            fheroes2::Copy( font[64], 4, 3, font[64], 5, 1, 1, 1 );
            fheroes2::Copy( font[64], 8, 6, font[64], 5, 0, 1, 1 );
            fheroes2::Copy( font[64], 8, 6, font[64], 6, 1, 1, 1 );
            fheroes2::Copy( font[64], 4, 8, font[64], 6, 2, 1, 1 );
            font[64].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::updateShadow( font[64], { -1, 2 }, 2 );

            font[91] = font[28];

            font[92].resize( font[69].width(), font[69].height() + 3 );
            font[92].reset();
            fheroes2::Copy( font[69], 0, 0, font[92], 0, 3, font[69].width(), font[69].height() );
            fheroes2::Copy( font[3], 2, 0, font[92], 3, 0, 5, 2 );
            font[92].setPosition( font[69].x(), font[69].y() - 3 );
            fheroes2::updateShadow( font[92], { -1, 2 }, 2 );

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
            fheroes2::updateShadow( font[94], { -1, 2 }, 2 );

            font[95] = font[30];
        }
        // Small font.
        {
            std::vector<fheroes2::Sprite> & font = _icnVsSprite[ICN::SMALFONT];

            font[3].resize( font[79].width(), font[79].height() + 2 );
            font[3].reset();
            fheroes2::Copy( font[79], 0, 0, font[3], 0, 2, font[79].width(), font[79].height() );
            font[3].setPosition( font[79].x(), font[79].y() - 2 );
            fheroes2::Copy( font[3], 2, 2, font[3], 2, 0, 1, 1 );
            fheroes2::Copy( font[3], 2, 2, font[3], 4, 0, 1, 1 );
            fheroes2::updateShadow( font[3], { -1, 1 }, 2 );

            font[4].resize( font[85].width(), font[85].height() + 2 );
            font[4].reset();
            fheroes2::Copy( font[85], 0, 0, font[4], 0, 2, font[85].width(), font[85].height() );
            font[4].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::Copy( font[4], 1, 2, font[4], 3, 0, 1, 1 );
            fheroes2::Copy( font[4], 1, 2, font[4], 5, 0, 1, 1 );
            fheroes2::updateShadow( font[4], { -1, 1 }, 2 );

            font[6].resize( font[85].width(), font[85].height() + 2 );
            font[6].reset();
            fheroes2::Copy( font[85], 0, 0, font[6], 0, 2, font[85].width(), font[85].height() );
            font[6].setPosition( font[85].x(), font[85].y() - 2 );
            fheroes2::Copy( font[6], 1, 2, font[6], 4, 0, 1, 1 );
            fheroes2::updateShadow( font[6], { -1, 1 }, 2 );

            font[10].resize( font[65].width(), font[65].height() + 2 );
            font[10].reset();
            fheroes2::Copy( font[65], 0, 0, font[10], 0, 2, font[65].width(), font[65].height() );
            font[10].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::Copy( font[10], 2, 2, font[10], 2, 1, 1, 1 );
            fheroes2::Copy( font[10], 2, 2, font[10], 4, 1, 1, 1 );
            fheroes2::Copy( font[10], 2, 2, font[10], 3, 0, 1, 1 );
            fheroes2::updateShadow( font[10], { -1, 1 }, 2 );

            font[28] = font[73];
            fheroes2::FillTransform( font[28], 0, 0, font[28].width(), 2, 1 );
            fheroes2::Copy( font[28], 1, 2, font[28], 1, 0, 1, 1 );
            fheroes2::Copy( font[28], 1, 2, font[28], 3, 0, 1, 1 );
            fheroes2::updateShadow( font[28], { -1, 1 }, 2 );

            font[30] = font[28];

            font[32].resize( font[65].width(), font[65].height() + 2 );
            font[32].reset();
            fheroes2::Copy( font[65], 0, 0, font[32], 0, 2, font[65].width(), font[65].height() );
            font[32].setPosition( font[65].x(), font[65].y() - 2 );
            fheroes2::Copy( font[32], 2, 2, font[32], 3, 0, 1, 1 );
            fheroes2::Copy( font[32], 2, 2, font[32], 4, 1, 1, 1 );
            fheroes2::updateShadow( font[32], { -1, 1 }, 2 );

            font[62].resize( font[67].width(), font[67].height() + 2 );
            font[62].reset();
            fheroes2::Copy( font[67], 0, 0, font[62], 0, 0, font[67].width(), font[67].height() );
            fheroes2::Copy( font[62], 3, 4, font[62], 3, 5, 1, 1 );
            fheroes2::Copy( font[62], 3, 4, font[62], 2, 6, 1, 1 );
            fheroes2::updateShadow( font[62], { -1, 1 }, 2 );

            font[64].resize( font[69].width(), font[69].height() + 2 );
            font[64].reset();
            fheroes2::Copy( font[69], 0, 0, font[64], 0, 2, font[69].width(), font[69].height() );
            font[64].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[64], 2, 2, font[64], 2, 0, 1, 1 );
            fheroes2::Copy( font[64], 2, 2, font[64], 3, 1, 1, 1 );
            fheroes2::updateShadow( font[64], { -1, 1 }, 2 );

            font[91] = font[28];

            font[92].resize( font[69].width(), font[69].height() + 2 );
            font[92].reset();
            fheroes2::Copy( font[69], 0, 0, font[92], 0, 2, font[69].width(), font[69].height() );
            font[92].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[92], 2, 2, font[92], 3, 0, 1, 1 );
            fheroes2::Copy( font[92], 2, 2, font[92], 2, 1, 1, 1 );
            fheroes2::Copy( font[92], 2, 2, font[92], 4, 1, 1, 1 );
            fheroes2::updateShadow( font[92], { -1, 1 }, 2 );

            font[93] = font[28];

            font[94].resize( font[69].width(), font[69].height() + 2 );
            font[94].reset();
            fheroes2::Copy( font[69], 0, 0, font[94], 0, 2, font[69].width(), font[69].height() );
            font[94].setPosition( font[69].x(), font[69].y() - 2 );
            fheroes2::Copy( font[94], 2, 2, font[94], 4, 0, 1, 1 );
            fheroes2::Copy( font[94], 2, 2, font[94], 3, 1, 1, 1 );
            fheroes2::updateShadow( font[94], { -1, 1 }, 2 );

            font[95] = font[28];
        }
    }

    void generateAlphabet( const fheroes2::SupportedLanguage language )
    {
        switch ( language ) {
        case fheroes2::SupportedLanguage::Polish:
            generatePolishAlphabet();
            break;
        case fheroes2::SupportedLanguage::German:
            generateGermanAlphabet();
            break;
        case fheroes2::SupportedLanguage::French:
            generateFrenchAlphabet();
            break;
        case fheroes2::SupportedLanguage::Russian:
            generateRussianAlphabet();
            break;
        case fheroes2::SupportedLanguage::Italian:
            generateItalianAlphabet();
            break;
        default:
            // Add new language generation code!
            assert( 0 );
            break;
        }

        _icnVsSprite[ICN::YELLOW_FONT].clear();
        _icnVsSprite[ICN::YELLOW_SMALLFONT].clear();
        _icnVsSprite[ICN::GRAY_FONT].clear();
        _icnVsSprite[ICN::GRAY_SMALL_FONT].clear();
        _icnVsSprite[ICN::WHITE_LARGE_FONT].clear();
    }

    void invertTransparency( fheroes2::Image & image )
    {
        if ( image.singleLayer() ) {
            assert( 0 );
            return;
        }

        uint8_t * transform = image.transform();
        uint8_t * transformEnd = transform + image.width() * image.height();
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
}

namespace fheroes2
{
    namespace AGG
    {
        void LoadOriginalICN( int id )
        {
            const std::vector<uint8_t> & body = ::AGG::ReadChunk( ICN::GetString( id ) );

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

                const uint8_t * data = body.data() + headerSize + header1.offsetData;

                _icnVsSprite[id][i]
                    = decodeICNSprite( data, sizeData, header1.width, header1.height, static_cast<int16_t>( header1.offsetX ), static_cast<int16_t>( header1.offsetY ) );
            }
        }

        // Helper function for LoadModifiedICN
        void CopyICNWithPalette( int icnId, int originalIcnId, const PAL::PaletteType paletteType )
        {
            assert( icnId != originalIcnId );

            GetICN( originalIcnId, 0 ); // always avoid calling LoadOriginalICN directly

            _icnVsSprite[icnId] = _icnVsSprite[originalIcnId];
            const std::vector<uint8_t> & palette = PAL::GetPalette( paletteType );
            for ( size_t i = 0; i < _icnVsSprite[icnId].size(); ++i ) {
                ApplyPalette( _icnVsSprite[icnId][i], palette );
            }
        }

        bool LoadModifiedICN( int id )
        {
            switch ( id ) {
            case ICN::ROUTERED:
                CopyICNWithPalette( id, ICN::ROUTE, PAL::PaletteType::RED );
                return true;
            case ICN::FONT:
            case ICN::SMALFONT: {
                LoadOriginalICN( id );

                auto & imageArray = _icnVsSprite[id];

                if ( id == ICN::SMALFONT ) {
                    // Small font in official Polish GoG version has all letters to be shifted by 1 pixel lower.
                    const std::vector<uint8_t> & body = ::AGG::ReadChunk( ICN::GetString( id ) );
                    const uint32_t crc32 = fheroes2::calculateCRC32( body.data(), body.size() );
                    if ( crc32 == 0xE9EC7A63 ) {
                        for ( Sprite & letter : imageArray ) {
                            letter.setPosition( letter.x(), letter.y() - 1 );
                        }
                    }
                }

                if ( id == ICN::FONT ) {
                    // The original images contain an issue: image layer has value 50 which is '2' in UTF-8. We must correct these (only 3) places
                    for ( size_t i = 0; i < imageArray.size(); ++i ) {
                        ReplaceColorIdByTransformId( imageArray[i], 50, 2 );
                    }
                }

                // Some checks that we really have CP1251 font
                const int32_t verifiedFontWidth = ( id == ICN::FONT ) ? 19 : 12;
                if ( imageArray.size() == 162 && imageArray[121].width() == verifiedFontWidth ) {
                    // Engine expects that letter indexes correspond to charcode - 0x20.
                    // In case CP1251 font.icn contains sprites for chars 0x20-0x7F, 0xC0-0xDF, 0xA8, 0xE0-0xFF, 0xB8 (in that order).
                    // We rearrange sprites array for corresponding sprite indexes to charcode - 0x20.
                    imageArray.insert( imageArray.begin() + 96, 64, imageArray[0] );
                    std::swap( imageArray[136], imageArray[192] ); // Move sprites for chars 0xA8
                    std::swap( imageArray[152], imageArray[225] ); // and 0xB8 to it's places.
                    imageArray.pop_back();
                    imageArray.erase( imageArray.begin() + 192 );
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
            case ICN::BTNBATTLEONLY:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < static_cast<uint32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNNEWGM, 6 + i );
                    // clean the button
                    Image uniform( 83, 23 );
                    uniform.fill( ( i == 0 ) ? GetColorId( 216, 184, 152 ) : GetColorId( 184, 136, 96 ) );
                    Copy( uniform, 0, 0, out, 28, 18, uniform.width(), uniform.height() );
                    // add 'ba'
                    Blit( GetICN( ICN::BTNCMPGN, i ), 41 - i, 28, out, 30 - i, 13, 28, 14 );
                    // add 'tt'
                    Blit( GetICN( ICN::BTNNEWGM, i ), 25 - i, 13, out, 57 - i, 13, 13, 14 );
                    Blit( GetICN( ICN::BTNNEWGM, i ), 25 - i, 13, out, 70 - i, 13, 13, 14 );
                    // add 'le'
                    Blit( GetICN( ICN::BTNNEWGM, 6 + i ), 97 - i, 21, out, 83 - i, 13, 13, 14 );
                    Blit( GetICN( ICN::BTNNEWGM, 6 + i ), 86 - i, 21, out, 96 - i, 13, 13, 14 );
                    // add 'on'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 44 - i, 21, out, 40 - i, 28, 31, 14 );
                    // add 'ly'
                    Blit( GetICN( ICN::BTNHOTST, i ), 47 - i, 21, out, 71 - i, 28, 12, 13 );
                    Blit( GetICN( ICN::BTNHOTST, i ), 72 - i, 21, out, 84 - i, 28, 13, 13 );
                }
                return true;
            case ICN::NON_UNIFORM_GOOD_MIN_BUTTON:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < static_cast<uint32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::RECRUIT, 4 + i );
                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6 + i, out, 30 - 2 * i, 5 + i, 31, 15 );
                    // add 'IN'
                    Copy( GetICN( ICN::APANEL, 4 + i ), 23 - i, 22 + i, out, 33 - i, 6 + i, 8, 14 ); // letter 'I'
                    Copy( GetICN( ICN::APANEL, 4 + i ), 31 - i, 22 + i, out, 44 - i, 6 + i, 17, 14 ); // letter 'N'
                }
                return true;
            case ICN::SPELLS:
                LoadOriginalICN( id );
                _icnVsSprite[id].resize( 66 );
                for ( uint32_t i = 60; i < 66; ++i ) {
                    int originalIndex = 0;
                    if ( i == 60 ) // Mass Cure
                        originalIndex = 6;
                    else if ( i == 61 ) // Mass Haste
                        originalIndex = 14;
                    else if ( i == 62 ) // Mass Slow
                        originalIndex = 1;
                    else if ( i == 63 ) // Mass Bless
                        originalIndex = 7;
                    else if ( i == 64 ) // Mass Curse
                        originalIndex = 3;
                    else if ( i == 65 ) // Mass Shield
                        originalIndex = 15;

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
            case ICN::BATTLEWAIT:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TEXTBAR, 4 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 3, 8, out, 3, 1, 43, 14 );

                    // add 'wait'
                    const Sprite wait = Crop( GetICN( ICN::ADVBTNS, 8 + i ), 5, 4, 28, 28 );
                    Image resizedWait( wait.width() / 2, wait.height() / 2 );
                    Resize( wait, resizedWait );

                    Blit( resizedWait, 0, 0, out, ( out.width() - 14 ) / 2, 0, 14, 14 );
                }
                return true;
            case ICN::BUYMAX:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::WELLXTRA, i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 6, 2, 52, 14 );

                    // add 'max'
                    Blit( GetICN( ICN::RECRUIT, 4 + i ), 12, 6, out, 7, 3, 50, 12 );
                }
                return true;
            case ICN::BTNGIFT_GOOD:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TRADPOST, 17 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 6, 4, 72, 15 );

                    // add 'G'
                    Blit( GetICN( ICN::CPANEL, i ), 18 - i, 27, out, 20 - i, 4, 15, 15 );

                    // add 'I'
                    Blit( GetICN( ICN::APANEL, 4 + i ), 22 - i, 20, out, 36 - i, 4, 9, 15 );

                    // add 'F'
                    Blit( GetICN( ICN::APANEL, 4 + i ), 48 - i, 20, out, 46 - i, 4, 13, 15 );

                    // add 'T'
                    Blit( GetICN( ICN::CPANEL, 6 + i ), 59 - i, 21, out, 60 - i, 5, 14, 14 );
                }
                return true;
            case ICN::BTNGIFT_EVIL:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TRADPOSE, 17 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEME, 11 + i ), 10, 6, out, 6, 4, 72, 15 );

                    // add 'G'
                    Blit( GetICN( ICN::CPANELE, i ), 18 - i, 27, out, 20 - i, 4, 15, 15 );

                    // add 'I'
                    Blit( GetICN( ICN::APANELE, 4 + i ), 22 - i, 20, out, 36 - i, 4, 9, 15 );

                    // add 'F'
                    Blit( GetICN( ICN::APANELE, 4 + i ), 48 - i, 20, out, 46 - i, 4, 13, 15 );

                    // add 'T'
                    Blit( GetICN( ICN::CPANELE, 6 + i ), 59 - i, 21, out, 60 - i, 5, 14, 14 );
                }
                return true;
            case ICN::BTNCONFIG:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::NON_UNIFORM_GOOD_OKAY_BUTTON, i );

                    // add 'config'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 31 - i, 20, out, 10 - i, 4, 77, 16 );
                }
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
                LoadOriginalICN( ICN::TROLLMSL );
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
                    // The pixel pixel of the original sprite has a skip value
                    if ( !out.empty() && out.transform()[0] == 1 ) {
                        out.transform()[0] = 0;
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
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    ApplyPalette( _icnVsSprite[id][i], 2 );
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
                    out.setPosition( source.y(), source.x() );
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
                    for ( uint32_t i : { 0, 2 } ) {
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
            case ICN::NON_UNIFORM_GOOD_OKAY_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRG, 4 ), 6, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRG, 5 );
                _icnVsSprite[id][1].setPosition( 0, 0 );

                // fix transparent corners
                CopyTransformLayer( _icnVsSprite[id][1], _icnVsSprite[id][0] );
                return true;
            case ICN::NON_UNIFORM_GOOD_CANCEL_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRG, 6 ), 6, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRG, 7 );
                _icnVsSprite[id][1].setPosition( 0, 0 );

                // fix transparent corners
                CopyTransformLayer( _icnVsSprite[id][1], _icnVsSprite[id][0] );
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
            case ICN::NON_UNIFORM_EVIL_OKAY_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRE, 4 ), 4, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRE, 5 );
                _icnVsSprite[id][1].setPosition( 0, 0 );

                // fix transparent corners
                CopyTransformLayer( _icnVsSprite[id][1], _icnVsSprite[id][0] );
                return true;
            case ICN::NON_UNIFORM_EVIL_CANCEL_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRE, 6 ), 4, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRE, 7 );
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
            case ICN::UNIFORM_GOOD_MAX_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                // Generate background
                Image background( 60, 25 );
                Copy( GetICN( ICN::SYSTEM, 12 ), 0, 0, background, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEM, 12 ), 89, 0, background, 53, 0, 7, 25 );

                // Released button
                Image temp( 60, 25 );
                Copy( GetICN( ICN::SYSTEM, 11 ), 0, 0, temp, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEM, 11 ), 89, 0, temp, 53, 0, 7, 25 );
                Copy( GetICN( ICN::RECRUIT, 4 ), 9, 2, temp, 4, 2, 52, 19 );

                _icnVsSprite[id][0] = background;
                Blit( temp, _icnVsSprite[id][0] );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                // Pressed button
                _icnVsSprite[id][1] = background;
                Copy( GetICN( ICN::RECRUIT, 5 ), 9, 2, _icnVsSprite[id][1], 3, 2, 53, 19 );

                return true;
            }
            case ICN::UNIFORM_GOOD_MIN_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                // Generate background
                Image background( 60, 25 );
                Copy( GetICN( ICN::SYSTEM, 12 ), 0, 0, background, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEM, 12 ), 89, 0, background, 53, 0, 7, 25 );

                // Released button
                Image temp( 60, 25 );
                Copy( GetICN( ICN::SYSTEM, 11 ), 0, 0, temp, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEM, 11 ), 89, 0, temp, 53, 0, 7, 25 );
                Copy( GetICN( ICN::RECRUIT, 4 ), 9, 2, temp, 4, 2, 21, 19 ); // letter 'M'
                Copy( GetICN( ICN::APANEL, 4 ), 23, 21, temp, 28, 5, 8, 14 ); // letter 'I'
                Copy( GetICN( ICN::APANEL, 4 ), 31, 21, temp, 39, 5, 17, 14 ); // letter 'N'

                _icnVsSprite[id][0] = background;
                Blit( temp, _icnVsSprite[id][0] );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                // Pressed button
                _icnVsSprite[id][1] = background;
                Copy( GetICN( ICN::RECRUIT, 5 ), 9, 3, _icnVsSprite[id][1], 3, 3, 19, 17 ); // letter 'M'
                Copy( GetICN( ICN::APANEL, 5 ), 21, 22, _icnVsSprite[id][1], 25, 6, 8, 14 ); // letter 'I'
                Copy( GetICN( ICN::APANEL, 5 ), 30, 21, _icnVsSprite[id][1], 37, 5, 17, 15 ); // letter 'N'

                return true;
            }
            case ICN::UNIFORM_EVIL_MAX_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                // Generate background
                Image background( 60, 25 );
                Copy( GetICN( ICN::SYSTEME, 12 ), 0, 0, background, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEME, 12 ), 89, 0, background, 53, 0, 7, 25 );

                // Released button
                Image temp( 60, 25 );
                Copy( GetICN( ICN::SYSTEME, 11 ), 0, 0, temp, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEME, 11 ), 89, 0, temp, 53, 0, 7, 25 );
                Copy( GetICN( ICN::CPANELE, 0 ), 46, 28, temp, 6, 5, 19, 14 ); // letter 'M'
                Copy( GetICN( ICN::CSPANBTE, 0 ), 49, 5, temp, 25, 5, 13, 14 ); // letter 'A'
                Copy( GetICN( ICN::CSPANBTE, 0 ), 62, 10, temp, 38, 10, 2, 9 ); // rest of letter 'A'
                Copy( GetICN( ICN::LGNDXTRE, 4 ), 28, 5, temp, 41, 5, 15, 14 ); // letter 'X'

                _icnVsSprite[id][0] = background;
                Blit( temp, _icnVsSprite[id][0] );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                // Pressed button
                _icnVsSprite[id][1] = background;
                Copy( GetICN( ICN::CPANELE, 1 ), 45, 29, _icnVsSprite[id][1], 4, 6, 19, 14 ); // letter 'M'
                Copy( GetICN( ICN::CSPANBTE, 1 ), 49, 6, _icnVsSprite[id][1], 23, 6, 12, 14 ); // letter 'A'
                Copy( GetICN( ICN::CSPANBTE, 1 ), 61, 11, _icnVsSprite[id][1], 35, 11, 3, 9 ); // rest of letter 'A'
                Copy( GetICN( ICN::LGNDXTRE, 5 ), 26, 4, _icnVsSprite[id][1], 38, 4, 15, 16 ); // letter 'X'
                _icnVsSprite[id][1].image()[353] = 21;
                _icnVsSprite[id][1].image()[622] = 21;
                _icnVsSprite[id][1].image()[964] = 21;

                return true;
            }
            case ICN::UNIFORM_EVIL_MIN_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                // Generate background
                Image background( 60, 25 );
                Copy( GetICN( ICN::SYSTEME, 12 ), 0, 0, background, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEME, 12 ), 89, 0, background, 53, 0, 7, 25 );

                // Released button
                Image temp( 60, 25 );
                Copy( GetICN( ICN::SYSTEME, 11 ), 0, 0, temp, 0, 0, 53, 25 );
                Copy( GetICN( ICN::SYSTEME, 11 ), 89, 0, temp, 53, 0, 7, 25 );
                Copy( GetICN( ICN::CPANELE, 0 ), 46, 28, temp, 6, 5, 19, 14 ); // letter 'M'
                Copy( GetICN( ICN::APANELE, 4 ), 23, 21, temp, 28, 5, 8, 14 ); // letter 'I'
                Copy( GetICN( ICN::APANELE, 4 ), 31, 21, temp, 39, 5, 17, 14 ); // letter 'N'

                _icnVsSprite[id][0] = background;
                Blit( temp, _icnVsSprite[id][0] );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                // Pressed button
                _icnVsSprite[id][1] = background;
                Copy( GetICN( ICN::CPANELE, 1 ), 45, 29, _icnVsSprite[id][1], 4, 6, 19, 14 ); // letter 'M'
                Copy( GetICN( ICN::APANELE, 5 ), 21, 22, _icnVsSprite[id][1], 25, 6, 8, 14 ); // letter 'I'
                Copy( GetICN( ICN::APANELE, 5 ), 30, 21, _icnVsSprite[id][1], 37, 5, 17, 15 ); // letter 'N'
                _icnVsSprite[id][1].image()[622] = 21;
                _icnVsSprite[id][1].image()[964] = 21;
                _icnVsSprite[id][1].image()[1162] = 21;

                return true;
            }
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
                std::vector<Image> input( 4 );

                const int32_t width = 45;
                const int32_t height = 20;

                for ( Image & image : input )
                    image.resize( width, height );

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
            case ICN::HEROES:
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    // This is the main menu image which shouldn't have any transform layer.
                    _icnVsSprite[id][0]._disableTransformLayer();
                }
                return true;
            case ICN::TOWNBKG3:
                // Warlock town background image contains 'empty' pixels leading to appear them as black.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 640 && original.height() == 256 ) {
                        replaceTranformPixel( original, 51945, 17 );
                        replaceTranformPixel( original, 61828, 25 );
                        replaceTranformPixel( original, 64918, 164 );
                        replaceTranformPixel( original, 77685, 18 );
                        replaceTranformPixel( original, 84618, 19 );
                    }
                }
                return true;
            case ICN::PORT0091:
                // Barbarian captain has one bad pixel.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 101 && original.height() == 93 ) {
                        replaceTranformPixel( original, 9084, 77 );
                    }
                }
                return true;
            case ICN::PORT0090:
                // Knight captain has multiple bad pixels.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 101 && original.height() == 93 ) {
                        replaceTranformPixel( original, 2314, 70 );
                        replaceTranformPixel( original, 5160, 71 );
                        replaceTranformPixel( original, 5827, 18 );
                        replaceTranformPixel( original, 7474, 167 );
                    }
                }
                return true;
            case ICN::CSTLWZRD:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 8 ) {
                    // Statue image has bad pixels.
                    Sprite & original = _icnVsSprite[id][7];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        replaceTranformPixel( original, 3687, 50 );
                        replaceTranformPixel( original, 5159, 108 );
                        replaceTranformPixel( original, 5294, 108 );
                    }
                }
                if ( _icnVsSprite[id].size() >= 24 ) {
                    // Mage tower image has a bad pixel.
                    Sprite & original = _icnVsSprite[id][23];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        replaceTranformPixel( original, 4333, 23 );
                    }
                }
                if ( _icnVsSprite[id].size() >= 29 ) {
                    // Mage tower image has a bad pixel.
                    Sprite & original = _icnVsSprite[id][28];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        replaceTranformPixel( original, 4333, 23 );
                    }
                }
                return true;
            case ICN::CSTLCAPK:
                // Knight captain has a bad pixel.
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 2 ) {
                    Sprite & original = _icnVsSprite[id][1];
                    if ( original.width() == 84 && original.height() == 81 ) {
                        replaceTranformPixel( original, 4934, 18 );
                    }
                }
                return true;
            case ICN::CSTLCAPW:
                // Warlock captain quarters have bad pixels.
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    Sprite & original = _icnVsSprite[id][0];
                    if ( original.width() == 84 && original.height() == 81 ) {
                        replaceTranformPixel( original, 1692, 26 );
                        replaceTranformPixel( original, 2363, 32 );
                        replaceTranformPixel( original, 2606, 21 );
                        replaceTranformPixel( original, 2608, 21 );
                    }
                }
                return true;
            case ICN::CSTLSORC:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 14 ) {
                    // Rainbow has bad pixels.
                    Sprite & original = _icnVsSprite[id][13];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        replaceTranformPixel( original, 2047, 160 );
                        replaceTranformPixel( original, 2052, 159 );
                        replaceTranformPixel( original, 2055, 160 );
                        replaceTranformPixel( original, 2060, 67 );
                        replaceTranformPixel( original, 2063, 159 );
                        replaceTranformPixel( original, 2067, 67 );
                        replaceTranformPixel( original, 2184, 67 );
                        replaceTranformPixel( original, 2192, 158 );
                        replaceTranformPixel( original, 3508, 67 );
                        replaceTranformPixel( original, 3641, 67 );
                        replaceTranformPixel( original, 3773, 69 );
                        replaceTranformPixel( original, 3910, 67 );
                        replaceTranformPixel( original, 4039, 69 );
                        replaceTranformPixel( original, 4041, 67 );
                        replaceTranformPixel( original, 4172, 67 );
                        replaceTranformPixel( original, 4578, 69 );
                    }
                }
                if ( _icnVsSprite[id].size() >= 25 ) {
                    // Red tower has bad pixels.
                    Sprite & original = _icnVsSprite[id][24];
                    if ( original.width() == 135 && original.height() == 57 ) {
                        replaceTranformPixel( original, 2830, 165 );
                        replaceTranformPixel( original, 3101, 165 );
                        replaceTranformPixel( original, 3221, 69 );
                    }
                }
                return true;
            case ICN::CURSOR_ADVENTURE_MAP: {
                // Create needed numbers
                const std::vector<Point> twoPoints = { { 2, 1 }, { 3, 1 }, { 1, 2 }, { 4, 2 }, { 3, 3 }, { 2, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 }, { 4, 5 } };
                const std::vector<Point> threePoints = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 } };
                const std::vector<Point> fourPoints = { { 1, 1 }, { 3, 1 }, { 1, 2 }, { 3, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 3 }, { 3, 4 }, { 3, 5 } };
                const std::vector<Point> fivePoints
                    = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 4, 4 }, { 1, 5 }, { 2, 5 }, { 3, 5 } };
                const std::vector<Point> sixPoints = { { 2, 1 }, { 3, 1 }, { 1, 2 }, { 1, 3 }, { 2, 3 }, { 3, 3 }, { 1, 4 }, { 4, 4 }, { 2, 5 }, { 3, 5 } };
                const std::vector<Point> sevenPoints = { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 4, 2 }, { 3, 3 }, { 2, 4 }, { 2, 5 } };
                const std::vector<Point> plusPoints = { { 2, 1 }, { 1, 2 }, { 2, 2 }, { 3, 2 }, { 2, 3 } };

                std::vector<Image> digits( 7 );
                digits[0] = createDigit( 6, 7, twoPoints );
                digits[1] = createDigit( 6, 7, threePoints );
                digits[2] = createDigit( 6, 7, fourPoints );
                digits[3] = createDigit( 6, 7, fivePoints );
                digits[4] = createDigit( 6, 7, sixPoints );
                digits[5] = createDigit( 6, 7, sevenPoints );
                digits[6] = addDigit( digits[5], createDigit( 5, 5, plusPoints ), { -1, -1 } );

                _icnVsSprite[id].reserve( 7 * 8 );

                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 4 ), digits, { -2, 1 } );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 5 ), digits, { 1, 1 } );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 6 ), digits, { 0, 1 } );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 7 ), digits, { -2, 1 } );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 8 ), digits, { 1, 1 } );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 9 ), digits, { -6, 1 } );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 28 ), digits, { 0, 1 } );

                return true;
            }
            case ICN::DISMISS_HERO_DISABLED_BUTTON:
            case ICN::NEW_CAMPAIGN_DISABLED_BUTTON:
            case ICN::MAX_DISABLED_BUTTON: {
                _icnVsSprite[id].resize( 1 );
                Sprite & output = _icnVsSprite[id][0];

                int buttonIcnId = ICN::UNKNOWN;
                uint32_t startIcnId = 0;

                if ( id == ICN::DISMISS_HERO_DISABLED_BUTTON ) {
                    buttonIcnId = ICN::HSBTNS;
                    startIcnId = 0;
                }
                else if ( id == ICN::NEW_CAMPAIGN_DISABLED_BUTTON ) {
                    buttonIcnId = ICN::BTNNEWGM;
                    startIcnId = 2;
                }
                else if ( id == ICN::MAX_DISABLED_BUTTON ) {
                    buttonIcnId = ICN::RECRUIT;
                    startIcnId = 4;
                }

                assert( buttonIcnId != ICN::UNKNOWN ); // Did you add a new disabled button and forget to add the condition above?

                const Sprite & released = GetICN( buttonIcnId, startIcnId );
                const Sprite & pressed = GetICN( buttonIcnId, startIcnId + 1 );
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
                        Copy( out, 0, 0, out, 15, 0, 1, 16 );
                    }
                }
                return true;
            case ICN::MAP_TYPE_ICON: {
                // TODO: add a new icon for the Resurrection add-on map type.
                _icnVsSprite[id].resize( 2 );
                for ( Sprite & icon : _icnVsSprite[id] ) {
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

                const int releasedIndex = ( id == ICN::GOOD_ARMY_BUTTON ) ? 0 : 4;
                _icnVsSprite[id][0] = GetICN( ICN::ADVBTNS, releasedIndex );
                _icnVsSprite[id][1] = GetICN( ICN::ADVBTNS, releasedIndex + 1 );
                AddTransparency( _icnVsSprite[id][0], 36 );
                AddTransparency( _icnVsSprite[id][1], 36 );
                AddTransparency( _icnVsSprite[id][1], 61 ); // remove the extra brown border

                return true;
            }
            case ICN::EVIL_ARMY_BUTTON:
            case ICN::EVIL_MARKET_BUTTON: {
                _icnVsSprite[id].resize( 2 );

                const int releasedIndex = ( id == ICN::EVIL_ARMY_BUTTON ) ? 0 : 4;
                _icnVsSprite[id][0] = GetICN( ICN::ADVEBTNS, releasedIndex );
                AddTransparency( _icnVsSprite[id][0], 36 );

                Sprite pressed = GetICN( ICN::ADVEBTNS, releasedIndex + 1 );
                AddTransparency( pressed, 36 );
                AddTransparency( pressed, 61 ); // remove the extra brown border

                _icnVsSprite[id][1] = Sprite( pressed.width(), pressed.height(), pressed.x(), pressed.y() );
                _icnVsSprite[id][1].reset();

                // put back pixels that actually should be black
                Fill( _icnVsSprite[id][1], 1, 4, 31, 31, 36 );
                Blit( pressed, _icnVsSprite[id][1] );

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
                    for ( uint32_t i : { 16, 18 } ) {
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
                if ( _icnVsSprite[id].size() >= 34 ) {
                    // Fix extra column at the end of AI controlled player.
                    for ( size_t i = 27; i < 34; ++i ) {
                        if ( _icnVsSprite[id][i].width() == 62 && _icnVsSprite[id][i].height() == 58 ) {
                            Copy( _icnVsSprite[id][i], 58, 44, _icnVsSprite[id][i], 59, 44, 1, 11 );
                        }
                    }
                }
                if ( _icnVsSprite[id].size() >= 70 ) {
                    // fix transparent corners on pressed OKAY and CANCEL buttons
                    CopyTransformLayer( _icnVsSprite[id][66], _icnVsSprite[id][67] );
                    CopyTransformLayer( _icnVsSprite[id][68], _icnVsSprite[id][69] );
                }
                return true;
            }
            case ICN::HSBTNS: {
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 4 ) {
                    // extract the EXIT button without background
                    Sprite exitReleased = _icnVsSprite[id][2];
                    Sprite exitPressed = _icnVsSprite[id][3];

                    // make the border parts around EXIT button transparent
                    Image exitCommonMask = ExtractCommonPattern( { &exitReleased, &exitPressed } );
                    invertTransparency( exitCommonMask );

                    CopyTransformLayer( exitCommonMask, exitReleased );
                    CopyTransformLayer( exitCommonMask, exitPressed );

                    // fix DISMISS button: get the EXIT button, then slap the text back
                    Sprite & dismissReleased = _icnVsSprite[id][0];

                    Sprite tmpReleased = dismissReleased;
                    Blit( exitReleased, 0, 0, tmpReleased, 5, 0, 27, 120 );
                    Blit( dismissReleased, 9, 4, tmpReleased, 9, 4, 19, 110 );

                    dismissReleased = std::move( tmpReleased );

                    Sprite & dismissPressed = _icnVsSprite[id][1];

                    // start with the released state as well to capture more details
                    Sprite tmpPressed = dismissReleased;
                    Blit( exitPressed, 0, 0, tmpPressed, 5, 0, 27, 120 );
                    Blit( dismissPressed, 9, 5, tmpPressed, 8, 5, 19, 110 );

                    dismissPressed = std::move( tmpPressed );
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
            default:
                break;
            }

            return false;
        }

        size_t GetMaximumICNIndex( int id )
        {
            if ( _icnVsSprite[id].empty() && !LoadModifiedICN( id ) ) {
                LoadOriginalICN( id );
            }

            return _icnVsSprite[id].size();
        }

        size_t GetMaximumTILIndex( int id )
        {
            if ( _tilVsImage[id].empty() ) {
                _tilVsImage[id].resize( 4 ); // 4 possible sides

                const std::vector<uint8_t> & data = ::AGG::ReadChunk( tilFileName[id] );
                if ( data.size() < headerSize ) {
                    // The important resource is absent! Make sure that you are using the correct version of the game.
                    assert( 0 );
                    return 0;
                }

                StreamBuf buffer( data );

                const uint32_t count = buffer.getLE16();
                const uint32_t width = buffer.getLE16();
                const uint32_t height = buffer.getLE16();
                const uint32_t size = width * height;
                if ( headerSize + count * size != data.size() ) {
                    return 0;
                }

                std::vector<Image> & originalTIL = _tilVsImage[id][0];

                originalTIL.resize( count );
                for ( uint32_t i = 0; i < count; ++i ) {
                    Image & tilImage = originalTIL[i];
                    tilImage.resize( width, height );
                    tilImage._disableTransformLayer();
                    memcpy( tilImage.image(), data.data() + headerSize + i * size, size );
                    std::fill( tilImage.transform(), tilImage.transform() + width * height, static_cast<uint8_t>( 0 ) );
                }

                for ( uint32_t shapeId = 1; shapeId < 4; ++shapeId ) {
                    std::vector<Image> & currentTIL = _tilVsImage[id][shapeId];
                    currentTIL.resize( count );

                    const bool horizontalFlip = ( shapeId & 2 ) != 0;
                    const bool verticalFlip = ( shapeId & 1 ) != 0;

                    for ( uint32_t i = 0; i < count; ++i ) {
                        currentTIL[i] = Flip( originalTIL[i], horizontalFlip, verticalFlip );
                    }
                }
            }

            return _tilVsImage[id][0].size();
        }

        // We have few ICNs which we need to scale like some related to main screen
        bool IsScalableICN( int id )
        {
            return id == ICN::HEROES || id == ICN::BTNSHNGL || id == ICN::SHNGANIM;
        }

        const Sprite & GetScaledICN( int icnId, uint32_t index )
        {
            const Sprite & originalIcn = _icnVsSprite[icnId][index];

            if ( Display::DEFAULT_WIDTH == Display::instance().width() && Display::DEFAULT_HEIGHT == Display::instance().height() ) {
                return originalIcn;
            }

            if ( _icnVsScaledSprite[icnId].empty() ) {
                _icnVsScaledSprite[icnId].resize( _icnVsSprite[icnId].size() );
            }

            Sprite & resizedIcn = _icnVsScaledSprite[icnId][index];

            const double scaleFactorX = static_cast<double>( Display::instance().width() ) / Display::DEFAULT_WIDTH;
            const double scaleFactorY = static_cast<double>( Display::instance().height() ) / Display::DEFAULT_HEIGHT;

            const int32_t resizedWidth = static_cast<int32_t>( originalIcn.width() * scaleFactorX + 0.5 );
            const int32_t resizedHeight = static_cast<int32_t>( originalIcn.height() * scaleFactorY + 0.5 );
            // Resize only if needed
            if ( resizedIcn.width() != resizedWidth || resizedIcn.height() != resizedHeight ) {
                resizedIcn.resize( resizedWidth, resizedHeight );
                resizedIcn.setPosition( static_cast<int32_t>( originalIcn.x() * scaleFactorX + 0.5 ), static_cast<int32_t>( originalIcn.y() * scaleFactorY + 0.5 ) );
                Resize( originalIcn, resizedIcn, false );
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

        const Sprite & GetLetter( uint32_t character, uint32_t fontType )
        {
            if ( character < 0x21 ) {
                return errorImage;
            }

            // TODO: correct naming and standartise the code
            switch ( fontType ) {
            case Font::GRAY_BIG:
                return GetICN( ICN::GRAY_FONT, character - 0x20 );
            case Font::GRAY_SMALL:
                return GetICN( ICN::GRAY_SMALL_FONT, character - 0x20 );
            case Font::YELLOW_BIG:
                return GetICN( ICN::YELLOW_FONT, character - 0x20 );
            case Font::YELLOW_SMALL:
                return GetICN( ICN::YELLOW_SMALLFONT, character - 0x20 );
            case Font::BIG:
                return GetICN( ICN::FONT, character - 0x20 );
            case Font::SMALL:
                return GetICN( ICN::SMALFONT, character - 0x20 );
            case Font::WHITE_LARGE:
                return GetICN( ICN::WHITE_LARGE_FONT, character - 0x20 );
            default:
                break;
            }

            return GetICN( ICN::SMALFONT, character - 0x20 );
        }

        uint32_t ASCIILastSupportedCharacter( const uint32_t fontType )
        {
            switch ( fontType ) {
            case Font::BIG:
            case Font::GRAY_BIG:
            case Font::YELLOW_BIG:
            case Font::WHITE_LARGE:
                return static_cast<uint32_t>( GetMaximumICNIndex( ICN::FONT ) ) + 0x20 - 1;
            case Font::SMALL:
            case Font::GRAY_SMALL:
            case Font::YELLOW_SMALL:
                return static_cast<uint32_t>( GetMaximumICNIndex( ICN::SMALFONT ) ) + 0x20 - 1;
            default:
                return 0;
            }
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
            default:
                // Did you add a new font size? Add the corresponding logic for it!
                assert( 0 );
                break;
            }

            assert( 0 ); // Did you add a new font size? Please add implementation.

            return errorImage;
        }

        void updateAlphabet( const SupportedLanguage language, const bool loadOriginalAlphabet )
        {
            if ( loadOriginalAlphabet || !isAlphabetSupported( language ) ) {
                alphabetPreserver.restore();
            }
            else {
                alphabetPreserver.preserve();
                generateAlphabet( language );
            }
        }

        bool isAlphabetSupported( const SupportedLanguage language )
        {
            switch ( language ) {
            case SupportedLanguage::Polish:
            case SupportedLanguage::German:
            case SupportedLanguage::French:
            case SupportedLanguage::Italian:
            case SupportedLanguage::Russian:
                return true;
            default:
                break;
            }

            return false;
        }
    }
}
