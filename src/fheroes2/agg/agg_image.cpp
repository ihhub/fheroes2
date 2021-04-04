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

#include <cstring>
#include <map>
#include <vector>

#include "agg.h"
#include "agg_file.h"
#include "agg_image.h"
#include "icn.h"
#include "image.h"
#include "image_tool.h"
#include "pal.h"
#include "screen.h"
#include "text.h"
#include "til.h"

namespace fheroes2
{
    namespace AGG
    {
        std::vector<std::vector<fheroes2::Sprite> > _icnVsSprite( ICN::LASTICN );
        std::vector<std::vector<std::vector<fheroes2::Image> > > _tilVsImage( TIL::LASTTIL );
        const fheroes2::Sprite errorImage;

        const uint32_t headerSize = 6;

        std::map<int, std::vector<fheroes2::Sprite> > _icnVsScaledSprite;

        bool IsValidICNId( int id )
        {
            return id >= 0 && static_cast<size_t>( id ) < _icnVsSprite.size();
        }

        bool IsValidTILId( int id )
        {
            return id >= 0 && static_cast<size_t>( id ) < _tilVsImage.size();
        }

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
                LoadOriginalICN( id );
                // The original images contain an issue: image layer has value 50 which is '2' in UTF-8. We must correct these (only 3) places
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    ReplaceColorIdByTransformId( _icnVsSprite[id][i], 50, 2 );
                }
                return true;
            case ICN::YELLOW_FONT:
                CopyICNWithPalette( id, ICN::FONT, PAL::PaletteType::YELLOW_TEXT );
                return true;
            case ICN::YELLOW_SMALFONT:
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
            case ICN::BATTLESKIP: // a button
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
            case ICN::BATTLEWAIT: // a button
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
            case ICN::BUYMAX: // a button
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
            case ICN::BTNGIFT_GOOD: // a button
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
            case ICN::BTNGIFT_EVIL: // a button
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
            case ICN::BTNCONFIG: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::REQUESTS, 1 + i );

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
                        Sprite temp = out;
                        out.resize( out.width() + 1, out.height() );
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
                if ( _icnVsSprite[id].size() > 2 ) { // Ranger's sprite
                    const Sprite & source = _icnVsSprite[id][1];
                    Sprite & modified = _icnVsSprite[id][2];
                    Sprite temp( source.width(), source.height() + 1 );
                    temp.reset();
                    Copy( source, 0, 0, temp, 0, 1, source.width(), source.height() );
                    Blit( modified, 0, 0, temp, 1, 0, modified.width(), modified.height() );
                    modified = temp;
                    modified.setPosition( 0, 1 );
                }
                if ( _icnVsSprite[id].size() > 4 ) { // Veteran Pikeman's sprite
                    Sprite & modified = _icnVsSprite[id][4];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = temp;
                    Fill( modified, 7, 0, 4, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 6 ) { // Master Swordsman's sprite
                    Sprite & modified = _icnVsSprite[id][6];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = temp;
                    Fill( modified, 2, 0, 5, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 8 ) { // Champion's sprite
                    Sprite & modified = _icnVsSprite[id][8];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = temp;
                    Fill( modified, 12, 0, 5, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 44 ) { // Archimage's sprite
                    Sprite & modified = _icnVsSprite[id][44];
                    Sprite temp = _icnVsSprite[id][43];
                    Blit( modified, 0, 0, temp, 1, 0, modified.width(), modified.height() );
                    modified = temp;
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
                LoadOriginalICN( id );
                if ( !_icnVsSprite[id].empty() ) {
                    // Fix incorrect font color.
                    ReplaceColorId( _icnVsSprite[id][0], 28, 56 );
                }
                return true;
            case ICN::NON_UNIFORM_GOOD_OKAY_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRG, 4 ), 6, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRG, 5 );
                _icnVsSprite[id][1].setPosition( 0, 0 );
                return true;
            case ICN::NON_UNIFORM_GOOD_CANCEL_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRG, 6 ), 6, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRG, 7 );
                _icnVsSprite[id][1].setPosition( 0, 0 );
                return true;
            case ICN::NON_UNIFORM_EVIL_OKAY_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRE, 4 ), 4, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRE, 5 );
                _icnVsSprite[id][1].setPosition( 0, 0 );
                return true;
            case ICN::NON_UNIFORM_EVIL_CANCEL_BUTTON:
                _icnVsSprite[id].resize( 2 );
                _icnVsSprite[id][0] = Crop( GetICN( ICN::CAMPXTRE, 6 ), 4, 0, 96, 25 );
                _icnVsSprite[id][0].setPosition( 0, 0 );

                _icnVsSprite[id][1] = GetICN( ICN::CAMPXTRE, 7 );
                _icnVsSprite[id][1].setPosition( 0, 0 );
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
                LoadModifiedICN( ICN::FONT );
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

                const std::vector<uint8_t> & data = ::AGG::ReadChunk( TIL::GetString( id ) );
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
                return GetICN( ICN::YELLOW_SMALFONT, character - 0x20 );
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

        const Sprite & GetUnicodeLetter( uint32_t character, uint32_t fontType )
        {
            // TODO: Add Unicode character support
            return GetLetter( character, fontType );
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
    }
}
