/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "pal.h"
#include "../../tools/palette_h2.h"

namespace PAL
{
    const u8 yellow_text_table[PALETTE_SIZE]
        = {0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 114, 115, 115, 116, 117, 117, 118, 119, 119, 120, 121, 121, 122, 123, 123, 124, 125, 125, 126, 127, 127, 128,
           129, 129, 130, 130, 130, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
           0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
           0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
           0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
           0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
           0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
           0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

    const u8 white_text_table[PALETTE_SIZE]
        = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0};

    const u8 gray_text_table[PALETTE_SIZE]
        = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 36, 36, 36, 36, 36, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0};

    const u8 tan_table[PALETTE_SIZE]
        = {213, 213, 213, 213, 213, 213, 213, 213, 213, 213, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 200, 201, 203, 204, 206, 207,
           208, 209, 210, 211, 213, 213, 213, 213, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 199, 200, 201, 203, 204, 205, 207,
           207, 209, 210, 211, 212, 198, 198, 198, 198, 198, 201, 203, 205, 207, 208, 210, 210, 211, 212, 212, 213, 213, 213, 213, 213, 213, 213, 198, 198,
           198, 199, 201, 203, 204, 206, 207, 208, 209, 210, 211, 212, 213, 213, 213, 213, 213, 213, 213, 213, 213, 198, 198, 198, 198, 198, 198, 198, 198,
           198, 198, 198, 198, 198, 198, 198, 198, 198, 200, 201, 203, 204, 206, 207, 198, 198, 198, 198, 198, 198, 198, 198, 198, 200, 201, 203, 204, 206,
           207, 208, 209, 210, 212, 213, 213, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 199, 201, 203, 204, 206, 207, 208, 209, 211, 212,
           213, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 199, 201, 202, 204, 206, 207, 198, 198, 198, 198, 198,
           198, 198, 198, 198, 198, 198, 198, 198, 199, 202, 205, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 198, 200, 203, 204, 207, 208, 209, 201,
           203, 207, 209, 206, 209, 208, 198, 198, 207, 213, 198, 201, 206, 208, 213, 213, 213, 213, 213, 213, 213, 213, 213, 213};

    const u8 no_cycle_table[PALETTE_SIZE]
        = {0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
           29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,
           58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,
           87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
           116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
           145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173,
           174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
           203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 188, 188, 188, 188, 118, 118, 118, 118, 222, 223, 224, 225, 226, 227, 228, 229, 230, 69,
           69,  69,  69,  69,  69,  69,  69,  69,  69,  69,  242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

    const u8 gray_table[PALETTE_SIZE]
        = {36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
           10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23, 24, 25, 26, 26, 27, 28, 29, 31, 14, 16, 17, 18, 20, 21, 22, 24, 25, 26, 28,
           28, 29, 30, 31, 32, 32, 33, 33, 33, 34, 34, 16, 17, 18, 20, 21, 22, 23, 24, 25, 26, 27, 28, 30, 31, 32, 32, 33, 34, 35, 35, 36, 36, 36, 11, 11, 11,
           11, 11, 12, 12, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 24, 24, 25, 26, 27, 28, 29,
           31, 32, 32, 33, 11, 12, 12, 13, 14, 14, 16, 16, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 29, 30, 32, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14,
           15, 15, 16, 16, 17, 18, 19, 20, 21, 22, 23, 24, 26, 10, 10, 11, 11, 11, 12, 12, 12, 12, 14, 16, 17, 18, 20, 22, 24, 17, 10, 12, 15, 19, 10, 10, 15,
           17, 18, 20, 21, 22, 23, 25, 26, 27, 27, 24, 21, 22, 26, 26, 27, 36, 12, 18, 25, 19, 21, 24, 26, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36};

    const u8 red_table[PALETTE_SIZE]
        = {197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 180, 182, 184, 186, 208, 209, 210, 210, 211, 211, 212, 213, 213, 196, 197, 197, 197, 197, 197,
           197, 197, 197, 197, 197, 197, 197, 197, 180, 180, 182, 182, 184, 186, 208, 209, 209, 210, 210, 210, 211, 211, 193, 212, 212, 213, 213, 196, 197,
           197, 197, 197, 197, 197, 208, 209, 210, 211, 193, 212, 213, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 209, 210,
           210, 211, 212, 212, 213, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 180, 180, 180, 180, 180, 182, 182, 183,
           184, 185, 208, 188, 189, 190, 210, 191, 211, 193, 193, 194, 195, 196, 197, 182, 184, 208, 209, 209, 210, 211, 211, 212, 213, 213, 196, 197, 197,
           197, 197, 197, 197, 197, 197, 197, 209, 209, 210, 210, 211, 211, 193, 212, 213, 213, 213, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197,
           197, 180, 180, 180, 182, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
           203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 187, 211, 180, 180, 215, 191, 192, 193, 194, 213, 197, 197, 197, 197,
           197, 197, 197, 197, 197, 197, 197, 197, 211, 197, 211, 212, 196, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197, 197};

    const u8 brown_table[PALETTE_SIZE]
        = {50, 42, 42, 42, 42, 50, 50, 50, 50, 53, 42, 43, 43, 44, 44, 45, 46, 46, 47, 47, 48, 49, 50, 51, 52, 52, 53, 54, 55, 56, 57, 58, 60, 62, 62, 62, 62,
           37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 45, 46, 47, 48, 50, 51, 52, 54, 55, 56, 58,
           58, 59, 60, 60, 61, 62, 62, 62, 62, 62, 62, 47, 48, 49, 50, 51, 52, 53, 54, 55, 57, 58, 58, 60, 60, 62, 62, 62, 62, 62, 62, 62, 62, 62, 43, 43, 44,
           44, 44, 45, 45, 45, 46, 47, 48, 48, 49, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 44, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 53, 54, 55, 56, 57,
           58, 60, 62, 62, 43, 43, 44, 44, 45, 45, 46, 47, 47, 48, 49, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 60, 61, 43, 44, 46, 47, 48, 49, 50, 52, 53, 54,
           55, 57, 58, 60, 60, 62, 62, 62, 62, 62, 62, 62, 62, 44, 44, 45, 45, 46, 47, 48, 49, 50, 51, 53, 54, 55, 57, 58, 59, 54, 58, 62, 62, 42, 45, 49, 55,
           47, 49, 50, 51, 52, 53, 55, 56, 58, 51, 52, 55, 57, 54, 57, 56, 44, 48, 54, 62, 49, 51, 54, 57, 50, 50, 50, 50, 50, 50, 50, 50, 50, 42};

    const u8 mirror_image_table[PALETTE_SIZE]
        = {0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  10,  10,  10,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
           25,  26,  27,  28,  29,  30,  31,  32,  37,  37,  37,  37,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
           54,  55,  56,  57,  58,  63,  63,  63,  63,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  85,  85,
           85,  85,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 108, 108, 108, 108, 108, 109, 110, 111,
           112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 131, 131, 131, 131, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
           141, 142, 143, 144, 145, 146, 147, 152, 152, 152, 152, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
           170, 175, 175, 175, 175, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 198, 198, 198, 198, 198,
           199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 214, 215, 216, 217, 218, 219, 220, 221, 222, 222, 223, 224, 225, 226, 227, 228, 229, 231,
           232, 233, 234, 235, 236, 237, 238, 238, 239, 240, 242, 242, 243, 244, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

    std::vector<SDL_Color> standard_palette;
    std::vector<SDL_Color> yellow_text_palette;
    std::vector<SDL_Color> white_text_palette;
    std::vector<SDL_Color> gray_text_palette;
    std::vector<SDL_Color> red_palette;
    std::vector<SDL_Color> gray_palette;
    std::vector<SDL_Color> brown_palette;
    std::vector<SDL_Color> tan_palette;
    std::vector<SDL_Color> no_cycle_palette;
    std::vector<SDL_Color> mirror_image_palette;

    struct palmap_t
    {
        int type;
        std::vector<SDL_Color> * colors;
    };

    const palmap_t palmap[] = {{STANDARD, &standard_palette},
                               {YELLOW_TEXT, &yellow_text_palette},
                               {WHITE_TEXT, &white_text_palette},
                               {GRAY_TEXT, &gray_text_palette},
                               {RED, &red_palette},
                               {GRAY, &gray_palette},
                               {BROWN, &brown_palette},
                               {TAN, &tan_palette},
                               {NO_CYCLE, &no_cycle_palette},
                               {MIRROR_IMAGE, &mirror_image_palette}};

    const palmap_t * current_palette;

    void CreatePalette( std::vector<SDL_Color> & palette, const u8 * table )
    {
        palette.reserve( PALETTE_SIZE );

        for ( u32 ii = 0; ii < PALETTE_SIZE; ++ii ) {
            SDL_Color col = standard_palette[table[ii]];
            palette.push_back( col );
        }
    }

    const std::vector<CyclingColorSet> & GetCyclingColors()
    {
        static std::vector<CyclingColorSet> cycleSet;
        if ( cycleSet.empty() ) {
            const CyclingColorSet cycleData[] = {{0xD6, 4, false}, {0xDA, 4, false}, {0xE7, 5, true}, {0xEE, 4, false}, {0xF2, 4, false}};
            cycleSet.insert( cycleSet.begin(), cycleData, cycleData + sizeof( cycleData ) / sizeof( CyclingColorSet ) );
        }

        return cycleSet;
    }
}

void PAL::CreateStandardPalette()
{
    const u32 ncolors = ARRAY_COUNT( kb_pal ) / 3;
    standard_palette.reserve( ncolors );

    for ( u32 ii = 0; ii < ncolors; ++ii ) {
        u32 index = ii * 3;
        SDL_Color col;

        col.r = kb_pal[index] << 2;
        col.g = kb_pal[index + 1] << 2;
        col.b = kb_pal[index + 2] << 2;

        standard_palette.push_back( col );
    }
}

int PAL::CurrentPalette()
{
    return current_palette->type;
}

RGBA PAL::GetPaletteColor( u8 index )
{
    const std::vector<SDL_Color> & colors = *current_palette->colors;
    return index < colors.size() ? RGBA( colors[index].r, colors[index].g, colors[index].b ) : RGBA( 0, 0, 0 );
}

const std::vector<uint8_t> & PAL::GetPalette( int type )
{
    switch ( type ) {
    case YELLOW_TEXT: {
        static std::vector<uint8_t> palette( yellow_text_table, yellow_text_table + PALETTE_SIZE );
        return palette;
    }
    case WHITE_TEXT: {
        static std::vector<uint8_t> palette( white_text_table, white_text_table + PALETTE_SIZE );
        return palette;
    }
    case GRAY_TEXT: {
        static std::vector<uint8_t> palette( gray_text_table, gray_text_table + PALETTE_SIZE );
        return palette;
    }
    case RED: {
        static std::vector<uint8_t> palette( red_table, red_table + PALETTE_SIZE );
        return palette;
    }
    case GRAY: {
        static std::vector<uint8_t> palette( gray_table, gray_table + PALETTE_SIZE );
        return palette;
    }
    case BROWN: {
        static std::vector<uint8_t> palette( brown_table, brown_table + PALETTE_SIZE );
        return palette;
    }
    case TAN: {
        static std::vector<uint8_t> palette( tan_table, tan_table + PALETTE_SIZE );
        return palette;
    }
    case NO_CYCLE: {
        static std::vector<uint8_t> palette( no_cycle_table, no_cycle_table + PALETTE_SIZE );
        return palette;
    }
    case MIRROR_IMAGE: {
        static std::vector<uint8_t> palette( mirror_image_table, mirror_image_table + PALETTE_SIZE );
        return palette;
    }
    }

    static std::vector<uint8_t> empty;
    return empty;
}

std::vector<uint8_t> PAL::CombinePalettes( const std::vector<uint8_t> & first, const std::vector<uint8_t> & second )
{
    if ( first.size() != PALETTE_SIZE || second.size() != PALETTE_SIZE )
        return std::vector<uint8_t>();

    std::vector<uint8_t> combined( PALETTE_SIZE, 0 );

    for ( size_t i = 0; i < first.size(); ++i ) {
        combined[i] = second[first[i]];
    }

    return combined;
}

void PAL::SwapPalette( int type )
{
    current_palette = &palmap[type];
    Surface::SetDefaultPalette( &( *current_palette->colors )[0], static_cast<int>( current_palette->colors->size() ) );
}

void PAL::InitAllPalettes()
{
    CreateStandardPalette();
    CreatePalette( yellow_text_palette, yellow_text_table );
    CreatePalette( white_text_palette, white_text_table );
    CreatePalette( gray_text_palette, gray_text_table );
    CreatePalette( red_palette, red_table );
    CreatePalette( gray_palette, gray_table );
    CreatePalette( brown_palette, brown_table );
    CreatePalette( tan_palette, tan_table );
    CreatePalette( no_cycle_palette, no_cycle_table );
    CreatePalette( mirror_image_palette, mirror_image_table );
    SwapPalette( PAL::STANDARD );
}

void PAL::Clear()
{
    yellow_text_palette.clear();
    white_text_palette.clear();
    gray_text_palette.clear();
    red_palette.clear();
    gray_palette.clear();
    brown_palette.clear();
    tan_palette.clear();
    no_cycle_palette.clear();
    mirror_image_palette.clear();
    standard_palette.clear();
}
