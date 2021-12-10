/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = _icnVsSprite[icnId];

            original.resize( 103 );
            original[96] = original[65];
            original[97] = original[79];
            original[98] = original[85];
            original[99] = original[34];
            original[100] = original[33];
            original[101] = original[47];
            original[102] = original[53];
        }

        // TODO: modify newly added characters accordingly.
    }

    void generateFrenchAlphabet()
    {
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = _icnVsSprite[icnId];

            original.resize( 96 );
            original[3] = original[79];
            original[4] = original[85];
            original[6] = original[85];
            original[10] = original[65];
            original[28] = original[73];
            original[30] = original[73];
            original[32] = original[65];
            original[62] = original[67];
            original[64] = original[69];
            original[91] = original[73];
            original[92] = original[69];
            original[93] = original[73];
            original[94] = original[69];
            original[95] = original[73];
        }

        // TODO: modify newly added characters accordingly.
    }

    void generateRussianAlphabet()
    {
        for ( const int icnId : { ICN::FONT, ICN::SMALFONT } ) {
            std::vector<fheroes2::Sprite> & original = _icnVsSprite[icnId];

            original.resize( 96 );
            original.insert( original.end(), 128, original[0] );

            size_t offset = 0;

            original[168 - 32] = original[37 + offset];

            original[192 - 32] = original[33 + offset];
            original[193 - 32] = original[34 + offset];
            original[194 - 32] = original[34 + offset];
            original[195 - 32] = original[52 + offset];
            original[196 - 32] = original[36 + offset];
            original[197 - 32] = original[37 + offset];
            original[198 - 32] = original[56 + offset];
            original[199 - 32] = original[19];

            original[200 - 32] = fheroes2::Flip( original[46 + offset], true, false );
            original[200 - 32].setPosition( original[46 + offset].x(), original[46 + offset].y() );

            original[201 - 32] = original[200 - 32];

            original[202 - 32] = original[43 + offset];
            original[203 - 32] = original[44 + offset];
            original[204 - 32] = original[45 + offset];
            original[205 - 32] = original[40 + offset];
            original[206 - 32] = original[47 + offset];
            original[207 - 32] = original[52 + offset];
            original[208 - 32] = original[48 + offset];
            original[209 - 32] = original[35 + offset];
            original[210 - 32] = original[52 + offset];
            original[211 - 32] = original[57 + offset];
            original[212 - 32] = original[49 + offset];
            original[213 - 32] = original[56 + offset];
            original[214 - 32] = original[53 + offset];
            original[215 - 32] = original[20];
            original[216 - 32] = original[55 + offset];
            original[217 - 32] = original[55 + offset];
            original[218 - 32] = original[48 + offset];
            original[219 - 32] = original[48 + offset];
            original[220 - 32] = original[48 + offset];

            original[221 - 32] = fheroes2::Flip( original[39 + offset], true, false );
            original[221 - 32].setPosition( original[39 + offset].x(), original[39 + offset].y() );

            original[222 - 32] = original[47 + offset];

            original[223 - 32] = fheroes2::Flip( original[50 + offset], true, false );
            original[223 - 32].setPosition( original[50 + offset].x(), original[50 + offset].y() );

            offset = 32;

            original[184 - 32] = original[37 + offset];

            original[224 - 32] = original[33 + offset];
            original[225 - 32] = original[34 + offset];
            original[226 - 32] = original[34 + offset];
            original[227 - 32] = original[82];
            original[228 - 32] = original[36 + offset];
            original[229 - 32] = original[37 + offset];
            original[230 - 32] = original[56 + offset];
            original[231 - 32] = original[19];
            original[232 - 32] = original[46 + offset];
            original[233 - 32] = original[46 + offset];
            original[234 - 32] = original[43 + offset];
            original[235 - 32] = original[44 + offset];
            original[236 - 32] = original[45 + offset];
            original[237 - 32] = original[40 + offset];
            original[238 - 32] = original[47 + offset];
            original[239 - 32] = original[52 + offset];
            original[240 - 32] = original[48 + offset];
            original[241 - 32] = original[35 + offset];
            original[242 - 32] = original[77];
            original[243 - 32] = original[57 + offset];
            original[244 - 32] = original[49 + offset];
            original[245 - 32] = original[56 + offset];
            original[246 - 32] = original[53 + offset];
            original[247 - 32] = original[20];
            original[248 - 32] = original[55 + offset];
            original[249 - 32] = original[55 + offset];
            original[250 - 32] = original[48 + offset];
            original[251 - 32] = original[48 + offset];
            original[252 - 32] = original[48 + offset];
            original[253 - 32] = original[35 + offset];
            original[254 - 32] = original[47 + offset];
            original[255 - 32] = original[81];
        }

        // TODO: modify newly added characters accordingly.
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

        uint8_t * t = image.transform();
        uint8_t * tend = t + image.width() * image.height();
        for ( ; t != tend; ++t ) {
            if ( *t == 0 ) {
                *t = 1;
            }
            else if ( *t == 1 ) {
                *t = 0;
            }
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
                CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::YELLOW_TEXT );
                return true;
            case ICN::YELLOW_SMALLFONT:
                CopyICNWithPalette( id, ICN::SMALFONT, PAL::PaletteType::YELLOW_TEXT );
                return true;
            case ICN::GRAY_FONT:
                CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::GRAY_TEXT );
                return true;
            case ICN::GRAY_SMALL_FONT:
                CopyICNWithPalette( id, ICN::SMALFONT, PAL::PaletteType::GRAY_TEXT );
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
                        Sprite temp = addShadow( modified, Point( -1, 2 ), 2 );
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
                        Blit( out, 0, 1, tmp, 1, 0, tmp.width() - 1, tmp.height() - 1 );
                        CopyTransformLayer( _icnVsSprite[id][i], tmp );

                        out.reset();
                        Blit( tmp, 1, 0, out, 0, 1, tmp.width() - 1, tmp.height() - 1 );
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
            case ICN::SWAP_ARROW_LEFT_TO_RIGHT: {
                // Since the original game does not have such resources we could generate it from hero meeting sprite.
                const Sprite & original = GetICN( ICN::SWAPWIN, 0 );
                std::vector<Image> input( 4 );

                const int32_t width = 43;
                const int32_t height = 20;

                for ( Image & image : input )
                    image.resize( width, height );

                Copy( original, 297, 270, input[0], 0, 0, width, height );
                Copy( original, 295, 291, input[1], 0, 0, width, height );
                Copy( original, 297, 363, input[2], 0, 0, width, height );
                Copy( original, 295, 384, input[3], 0, 0, width, height );

                input[1] = Flip( input[1], true, false );
                input[3] = Flip( input[3], true, false );

                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = ExtractCommonPattern( input );
                Sprite & out = _icnVsSprite[id][0];

                // Here are 2 pixels which should be removed.
                if ( out.width() == 43 && out.height() == 20 ) {
                    if ( out.image()[38] != 0 ) {
                        out.image()[38] = 0;
                        out.transform()[38] = 1;
                    }
                    if ( out.image()[28 + 3 * 43] != 0 ) {
                        out.image()[28 + 3 * 43] = 0;
                        out.transform()[28 + 3 * 43] = 1;
                    }
                }

                _icnVsSprite[id][1] = _icnVsSprite[id][0];
                ApplyPalette( _icnVsSprite[id][1], 4 );

                _icnVsSprite[id][0] = addShadow( _icnVsSprite[id][0], Point( -3, 3 ), 3 );
                _icnVsSprite[id][1] = addShadow( _icnVsSprite[id][1], Point( -2, 2 ), 3 );
                _icnVsSprite[id][0].setPosition( -3, 0 );
                _icnVsSprite[id][1].setPosition( -2, 1 );

                return true;
            }
            case ICN::SWAP_ARROW_RIGHT_TO_LEFT: {
                // Since the original game does not have such resources we could generate it from hero meeting sprite.
                const Sprite & original = GetICN( ICN::SWAPWIN, 0 );
                std::vector<Image> input( 4 );

                const int32_t width = 43;
                const int32_t height = 20;

                for ( Image & image : input )
                    image.resize( width, height );

                Copy( original, 297, 270, input[0], 0, 0, width, height );
                Copy( original, 295, 291, input[1], 0, 0, width, height );
                Copy( original, 297, 363, input[2], 0, 0, width, height );
                Copy( original, 295, 384, input[3], 0, 0, width, height );

                input[1] = Flip( input[1], true, false );
                input[3] = Flip( input[3], true, false );

                _icnVsSprite[id].resize( 2 );
                Image temp = ExtractCommonPattern( input );

                // Here are 2 pixels which should be removed.
                if ( temp.width() == 43 && temp.height() == 20 ) {
                    if ( temp.image()[38] != 0 ) {
                        temp.image()[38] = 0;
                        temp.transform()[38] = 1;
                    }
                    if ( temp.image()[28 + 3 * 43] != 0 ) {
                        temp.image()[28 + 3 * 43] = 0;
                        temp.transform()[28 + 3 * 43] = 1;
                    }
                }

                _icnVsSprite[id][0] = Flip( temp, true, false );

                _icnVsSprite[id][1] = _icnVsSprite[id][0];
                ApplyPalette( _icnVsSprite[id][1], 4 );

                _icnVsSprite[id][0] = addShadow( _icnVsSprite[id][0], Point( -3, 3 ), 3 );
                _icnVsSprite[id][1] = addShadow( _icnVsSprite[id][1], Point( -2, 2 ), 3 );
                _icnVsSprite[id][0].setPosition( -3, 0 );
                _icnVsSprite[id][1].setPosition( -2, 1 );

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
                digits[6] = addDigit( digits[5], createDigit( 5, 5, plusPoints ), Point( -1, -1 ) );

                _icnVsSprite[id].reserve( 7 * 8 );

                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 4 ), digits, Point( -2, 1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 5 ), digits, Point( 1, 1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 6 ), digits, Point( 0, 1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 7 ), digits, Point( -2, 1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 8 ), digits, Point( 1, 1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 9 ), digits, Point( -6, 1 ) );
                populateCursorIcons( _icnVsSprite[id], GetICN( ICN::ADVMCO, 28 ), digits, Point( 0, 1 ) );

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

                std::vector<Image> dismissImages;
                dismissImages.emplace_back( released );
                dismissImages.emplace_back( pressed );

                Image common = ExtractCommonPattern( dismissImages );
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

                    for ( size_t i = 1; i < 7; i++ ) {
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
                    uint32_t color = id == ICN::SPANBTN || id == ICN::CSPANBTN ? 57 : 32;
                    DrawLine( released, Point( 0, 3 ), Point( 0, out.height() - 1 ), color );
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
                    // fix transparent corners on OKAY button
                    CopyTransformLayer( _icnVsSprite[id][9], _icnVsSprite[id][8] );
                }
                return true;
            }
            case ICN::HSBTNS: {
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() >= 4 ) {
                    // extract the EXIT button without background
                    Image exitReleased = _icnVsSprite[id][2];
                    Image exitPressed = _icnVsSprite[id][3];

                    // make the border parts around EXIT button transparent
                    Image exitCommonMask = ExtractCommonPattern( { exitReleased, exitPressed } );
                    invertTransparency( exitCommonMask );
                    CopyTransformLayer( exitCommonMask, exitReleased );
                    CopyTransformLayer( exitCommonMask, exitPressed );

                    // fix DISMISS button: get the EXIT button, then slap the text back
                    Sprite & outReleased = _icnVsSprite[id][0];

                    Sprite tmpReleased = outReleased;
                    Blit( exitReleased, 0, 0, tmpReleased, 5, 0, 27, 120 );
                    Blit( outReleased, 9, 4, tmpReleased, 9, 4, 19, 110 );

                    outReleased = std::move( tmpReleased );

                    Sprite & outPressed = _icnVsSprite[id][1];

                    Sprite tmpPressed = outPressed;
                    Blit( exitPressed, 0, 0, tmpPressed, 5, 0, 27, 120 );
                    Blit( outPressed, 9, 5, tmpPressed, 8, 5, 19, 110 );

                    outPressed = std::move( tmpPressed );
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
                    memcpy( tilImage.image(), data.data() + headerSize + i * size, size );
                    std::fill( tilImage.transform(), tilImage.transform() + width * height, 0 );
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
                    break;
                }
                break;
            case FontSize::LARGE:
                switch ( fontType.color ) {
                case FontColor::WHITE:
                    return GetICN( ICN::WHITE_LARGE_FONT, character - 0x20 );
                default:
                    break;
                }
                break;
            default:
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
                // TODO: uncomment the line below once Russian alphabet is good enough
                // case SupportedLanguage::Russian:
                return true;
            default:
                break;
            }

            return false;
        }
    }
}
