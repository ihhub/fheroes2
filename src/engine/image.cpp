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

#include "image.h"
#include "../../tools/palette_h2.h"

#include <cmath>
#include <cstring>

// FIXME: please remove me before merging to master branch
#include <cstdio>

namespace
{
    // 0 in shadow part means no shadow, 1 means to skip any drawing so to don't waste extra CPU cycles for tableId - 2 command we just add a fake extra tables
    const uint8_t transformTable[256 * 14] = {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,
        35,  36,  36,  36,  36,  36,  36,  36,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  62,
        62,  62,  62,  62,  62,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  84,  84,  84,  84,  84,  91,  92,
        93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 107, 107, 107, 107, 107, 107, 114, 115, 116, 117, 118, 119, 120, 121,
        122, 123, 124, 125, 126, 127, 128, 129, 130, 130, 130, 130, 130, 130, 130, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
        150, 151, 151, 151, 151, 151, 151, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 174, 174, 174, 174,
        174, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 197, 197, 197, 197, 197, 202, 203, 204, 205, 206,
        207, 208, 209, 210, 211, 212, 213, 213, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 225, 226, 227, 228, 229, 230, 230, 230, 230, 73,
        75,  77,  79,  81,  76,  78,  74,  76,  78,  80,  244, 245, 245, 245, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // First

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
        33,  34,  35,  36,  36,  36,  36,  36,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
        62,  62,  62,  62,  62,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  84,  84,  84,  89,  90,
        91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 107, 107, 107, 107, 112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 130, 130, 130, 130, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147,
        148, 149, 150, 151, 151, 151, 151, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 174, 174,
        174, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 197, 197, 197, 201, 202, 203, 204, 205,
        206, 207, 208, 209, 210, 211, 212, 213, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 224, 225, 226, 227, 228, 229, 230, 230, 230, 76,
        76,  76,  76,  76,  76,  76,  76,  76,  76,  78,  244, 245, 245, 245, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // Second

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
        31,  32,  33,  34,  35,  36,  36,  36,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
        60,  61,  62,  62,  62,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  84,  84,  87,  88,
        89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 107, 107, 110, 111, 112, 113, 114, 115, 116, 117,
        118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 130, 130, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
        147, 148, 149, 150, 151, 151, 151, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174,
        174, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 197, 197, 200, 201, 202, 203, 204,
        205, 206, 207, 208, 209, 210, 211, 212, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 223, 224, 225, 226, 227, 228, 229, 230, 230, 76,
        76,  76,  76,  76,  76,  76,  76,  76,  76,  76,  243, 244, 245, 245, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // Third

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
        30,  31,  32,  33,  34,  35,  36,  36,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,
        59,  60,  61,  62,  62,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  84,  86,  87,
        88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 107, 109, 110, 111, 112, 113, 114, 115, 116,
        117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 130, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145,
        146, 147, 148, 149, 150, 151, 151, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
        174, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 197, 199, 200, 201, 202, 203,
        204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 223, 224, 225, 226, 227, 228, 229, 230, 230, 75,
        75,  75,  75,  75,  75,  75,  75,  75,  75,  75,  243, 244, 245, 245, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // Fourth

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  10,  11,  11,  11,  12,  13,  13,  13,  14,  14,  15,  15,  15,  16,  17,  17,  17,  18,
        18,  19,  19,  20,  20,  20,  21,  21,  11,  37,  37,  37,  38,  38,  39,  39,  39,  40,  40,  41,  41,  41,  41,  42,  42,  19,  42,  20,  20,
        20,  20,  20,  20,  21,  12,  131, 63,  63,  63,  64,  64,  64,  65,  65,  65,  65,  65,  242, 242, 242, 242, 242, 242, 242, 242, 242, 13,  14,
        15,  15,  16,  85,  17,  85,  85,  85,  85,  19,  86,  20,  20,  20,  21,  21,  21,  21,  21,  21,  21,  10,  108, 108, 109, 109, 109, 110, 110,
        110, 110, 199, 40,  41,  41,  41,  41,  41,  42,  42,  42,  42,  20,  20,  11,  11,  131, 131, 132, 132, 132, 133, 133, 134, 134, 134, 135, 135,
        18,  136, 19,  19,  20,  20,  20,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  15,  15,  15,  16,  17,  17,  17,  18,  18,  19,  19,  20,
        20,  11,  175, 175, 176, 176, 38,  177, 177, 178, 178, 178, 179, 179, 179, 179, 180, 180, 180, 180, 180, 180, 21,  21,  108, 108, 38,  109, 38,
        109, 39,  40,  40,  41,  41,  41,  42,  42,  42,  20,  199, 179, 180, 180, 110, 110, 40,  42,  110, 110, 86,  86,  86,  86,  18,  18,  19,  65,
        65,  65,  66,  65,  66,  65,  152, 155, 65,  242, 15,  16,  17,  19,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // Fifth

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  11,  12,  12,  13,  13,  14,  15,  15,  16,  16,  17,  17,  18,  19,  20,  20,  21,
        21,  22,  22,  23,  24,  24,  25,  25,  37,  37,  38,  38,  39,  39,  40,  41,  41,  41,  42,  42,  43,  43,  44,  44,  45,  45,  46,  46,  23,
        24,  24,  24,  24,  24,  131, 63,  63,  64,  64,  65,  65,  66,  66,  242, 67,  67,  68,  68,  243, 243, 243, 243, 243, 243, 243, 243, 15,  15,
        85,  85,  85,  85,  86,  86,  87,  87,  88,  88,  88,  88,  89,  24,  90,  25,  25,  25,  25,  25,  25,  37,  108, 109, 109, 110, 110, 111, 111,
        200, 200, 201, 201, 42,  43,  43,  44,  44,  44,  45,  45,  46,  46,  46,  11,  131, 132, 132, 132, 133, 133, 134, 135, 135, 136, 242, 137, 137,
        138, 243, 243, 243, 243, 243, 24,  152, 152, 153, 153, 154, 154, 155, 156, 156, 157, 158, 158, 159, 18,  19,  19,  20,  20,  21,  22,  22,  23,
        24,  37,  175, 176, 176, 177, 177, 178, 179, 179, 180, 180, 180, 181, 181, 181, 182, 182, 182, 46,  47,  47,  48,  25,  108, 109, 109, 109, 198,
        199, 199, 201, 201, 42,  43,  43,  44,  45,  46,  46,  201, 181, 182, 183, 111, 111, 202, 45,  111, 111, 87,  88,  88,  88,  88,  21,  22,  66,
        66,  68,  68,  67,  68,  68,  152, 157, 66,  69,  16,  18,  20,  21,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // Sixth

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  11,  12,  13,  14,  14,  15,  16,  17,  17,  18,  19,  20,  20,  21,  22,  23,  24,
        24,  25,  26,  26,  27,  28,  29,  29,  37,  37,  38,  39,  40,  40,  41,  42,  42,  43,  44,  44,  45,  46,  46,  47,  47,  48,  48,  49,  50,
        50,  27,  28,  28,  28,  63,  63,  64,  65,  65,  66,  67,  67,  68,  69,  69,  69,  70,  70,  70,  244, 71,  244, 244, 244, 244, 245, 16,  85,
        85,  86,  87,  87,  88,  88,  89,  90,  90,  91,  91,  91,  92,  93,  93,  93,  29,  29,  29,  29,  29,  37,  109, 109, 110, 111, 111, 112, 113,
        112, 112, 203, 203, 203, 44,  45,  46,  47,  47,  47,  48,  48,  49,  50,  131, 131, 132, 133, 133, 134, 135, 136, 136, 137, 137, 139, 139, 139,
        141, 141, 141, 143, 143, 245, 245, 152, 152, 153, 154, 155, 155, 156, 157, 158, 158, 159, 160, 161, 162, 163, 163, 164, 165, 165, 166, 26,  26,
        27,  175, 13,  176, 177, 178, 178, 179, 180, 181, 181, 182, 182, 183, 183, 183, 184, 184, 185, 185, 50,  50,  52,  52,  109, 109, 198, 199, 200,
        201, 201, 202, 202, 44,  45,  46,  47,  48,  48,  49,  204, 205, 185, 185, 112, 112, 204, 47,  112, 113, 88,  89,  91,  92,  93,  93,  25,  66,
        68,  69,  69,  68,  69,  69,  153, 159, 68,  71,  18,  242, 243, 24,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // Seventh

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  12,  13,  13,  14,  15,  16,  17,  17,  19,  19,  20,  21,  22,  23,  24,  24,  26,
        26,  27,  28,  28,  30,  30,  31,  32,  37,  38,  39,  39,  40,  41,  42,  43,  43,  44,  45,  46,  46,  47,  48,  49,  50,  50,  51,  52,  52,
        53,  54,  54,  30,  31,  63,  64,  64,  65,  66,  67,  68,  69,  69,  70,  71,  71,  71,  72,  72,  72,  73,  73,  73,  168, 168, 168, 85,  85,
        86,  87,  88,  88,  89,  90,  91,  91,  92,  93,  93,  94,  95,  95,  96,  96,  96,  31,  32,  32,  32,  108, 109, 198, 110, 111, 112, 113, 113,
        113, 116, 117, 118, 119, 120, 121, 47,  48,  50,  50,  51,  51,  52,  52,  131, 132, 132, 133, 134, 135, 136, 137, 137, 138, 139, 140, 141, 141,
        143, 143, 144, 145, 146, 147, 30,  152, 153, 153, 154, 155, 156, 157, 158, 158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167, 168, 169, 28,
        29,  175, 176, 177, 177, 178, 179, 180, 181, 182, 182, 183, 184, 185, 185, 185, 186, 186, 187, 50,  52,  52,  54,  55,  109, 198, 199, 200, 201,
        202, 202, 204, 204, 205, 207, 47,  49,  50,  51,  52,  206, 206, 187, 188, 113, 113, 118, 49,  222, 222, 223, 224, 225, 226, 95,  227, 228, 67,
        68,  70,  71,  69,  71,  70,  153, 65,  69,  73,  242, 22,  243, 244, 0,   0,   0,   0,   0,   0,   0,   0,   0,
        0, // Eighth

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  11,  12,  12,  13,  14,  14,  15,  16,  16,  17,  18,  18,  19,  20,  242, 242, 22,
        22,  23,  243, 243, 25,  244, 244, 244, 11,  37,  38,  38,  39,  40,  40,  41,  178, 18,  19,  20,  20,  21,  21,  22,  22,  22,  22,  22,  23,
        23,  23,  23,  24,  244, 63,  63,  64,  64,  65,  65,  66,  66,  67,  67,  68,  68,  68,  69,  69,  69,  69,  69,  69,  70,  70,  70,  15,  15,
        16,  85,  86,  18,  19,  19,  20,  159, 21,  21,  161, 22,  163, 163, 163, 23,  23,  165, 165, 244, 244, 37,  108, 38,  109, 109, 110, 199, 200,
        199, 40,  41,  42,  42,  42,  43,  43,  44,  22,  22,  23,  23,  23,  23,  131, 131, 132, 132, 133, 133, 134, 134, 135, 135, 136, 136, 137, 137,
        138, 138, 139, 139, 140, 141, 244, 152, 152, 153, 153, 154, 154, 155, 155, 156, 156, 157, 158, 158, 159, 242, 159, 161, 161, 243, 243, 243, 243,
        164, 11,  175, 176, 176, 177, 177, 178, 179, 179, 180, 180, 181, 181, 182, 182, 182, 182, 183, 22,  23,  23,  23,  23,  108, 38,  38,  39,  39,
        40,  40,  41,  178, 180, 42,  44,  45,  23,  23,  23,  180, 181, 181, 183, 110, 200, 42,  45,  85,  86,  87,  87,  87,  21,  22,  22,  23,  66,
        66,  67,  68,  67,  68,  68,  153, 158, 67,  70,  64,  65,  242, 243, 159, 159, 159, 159, 159, 159, 159, 159, 159, 10, // Nineth

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  11,  12,  13,  14,  14,  15,  16,  16,  17,  18,  19,  19,  20,  242, 22,  22,  243,
        243, 243, 244, 244, 244, 244, 245, 245, 37,  37,  38,  176, 39,  177, 41,  41,  42,  179, 20,  180, 45,  22,  23,  23,  24,  24,  24,  24,  25,
        25,  25,  25,  26,  26,  63,  63,  64,  64,  65,  66,  67,  67,  68,  68,  69,  69,  70,  70,  70,  70,  70,  71,  71,  71,  71,  71,  15,  85,
        85,  86,  86,  87,  20,  88,  89,  22,  161, 162, 163, 163, 164, 164, 165, 165, 166, 166, 167, 167, 167, 37,  108, 109, 109, 110, 199, 111, 111,
        200, 201, 41,  43,  43,  43,  44,  44,  46,  46,  24,  24,  25,  25,  25,  131, 131, 132, 132, 133, 134, 135, 135, 136, 136, 137, 138, 138, 139,
        139, 140, 140, 141, 141, 142, 143, 152, 152, 153, 153, 154, 155, 155, 156, 156, 157, 158, 158, 159, 159, 161, 161, 162, 162, 163, 164, 244, 244,
        244, 175, 175, 176, 177, 177, 178, 179, 179, 180, 181, 181, 182, 182, 183, 183, 183, 184, 184, 184, 25,  25,  25,  25,  108, 109, 109, 39,  40,
        41,  41,  42,  42,  43,  43,  45,  46,  47,  25,  25,  181, 182, 183, 185, 111, 111, 42,  46,  111, 87,  88,  88,  88,  22,  23,  24,  25,  66,
        67,  68,  69,  68,  69,  69,  153, 0,   68,  71,  65,  242, 242, 243, 0,   0,   0,   0,   0,   0,   0,   0,   0,   10, // Tenth

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  11,  13,  13,  14,  15,  16,  16,  17,  18,  19,  20,  21,  21,  22,  23,  243, 25,
        244, 244, 244, 28,  245, 245, 245, 31,  37,  38,  38,  39,  40,  41,  41,  42,  42,  180, 45,  46,  46,  47,  47,  25,  26,  26,  26,  26,  27,
        27,  27,  27,  27,  245, 63,  63,  64,  65,  66,  66,  67,  68,  69,  69,  70,  70,  71,  71,  72,  72,  72,  72,  72,  73,  73,  73,  16,  85,
        85,  86,  87,  88,  88,  90,  90,  91,  91,  163, 164, 164, 165, 166, 166, 167, 167, 168, 169, 169, 170, 37,  108, 109, 198, 199, 111, 112, 112,
        201, 202, 202, 43,  44,  45,  45,  46,  46,  47,  48,  26,  27,  27,  27,  131, 131, 132, 133, 134, 135, 135, 136, 137, 137, 138, 139, 139, 140,
        141, 141, 142, 143, 143, 144, 145, 152, 152, 153, 154, 155, 155, 156, 156, 158, 158, 159, 160, 160, 161, 162, 163, 164, 164, 165, 166, 166, 167,
        167, 175, 176, 176, 177, 178, 178, 179, 180, 181, 182, 182, 183, 184, 184, 184, 185, 186, 186, 50,  51,  27,  27,  27,  109, 109, 109, 40,  40,
        41,  42,  43,  43,  44,  45,  46,  47,  49,  27,  27,  182, 183, 184, 187, 112, 112, 43,  47,  112, 87,  89,  90,  91,  91,  24,  26,  26,  67,
        68,  69,  70,  69,  70,  70,  153, 0,   0,   73,  65,  242, 243, 244, 0,   0,   0,   0,   0,   0,   0,   0,   0,
        10, // Eleventh

        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   10,  11,  12,  13,  13,  14,  15,  16,  17,  18,  19,  20,  21,  21,  23,  243, 24,  25,  244,
        244, 27,  245, 245, 31,  170, 149, 149, 37,  38,  38,  39,  40,  41,  42,  42,  44,  45,  46,  46,  47,  48,  49,  50,  51,  28,  28,  28,  29,
        29,  29,  29,  29,  30,  63,  64,  65,  65,  66,  67,  68,  69,  70,  70,  71,  72,  72,  73,  73,  74,  74,  75,  75,  76,  76,  76,  85,  85,
        86,  87,  88,  89,  90,  91,  91,  92,  93,  93,  166, 166, 96,  168, 168, 169, 170, 170, 171, 171, 171, 37,  109, 109, 110, 200, 111, 112, 113,
        202, 202, 203, 44,  45,  46,  46,  47,  48,  49,  50,  51,  52,  29,  29,  131, 132, 133, 133, 134, 135, 136, 137, 138, 139, 139, 140, 141, 142,
        142, 144, 144, 145, 145, 146, 147, 152, 153, 153, 154, 155, 156, 157, 158, 159, 159, 160, 161, 162, 163, 164, 164, 165, 166, 167, 168, 168, 168,
        169, 175, 176, 177, 177, 178, 179, 180, 181, 182, 182, 183, 184, 185, 186, 186, 187, 187, 189, 189, 193, 193, 146, 146, 109, 109, 198, 199, 201,
        201, 201, 44,  205, 45,  46,  47,  48,  50,  52,  29,  183, 185, 186, 189, 112, 112, 205, 49,  222, 88,  89,  91,  92,  93,  26,  27,  28,  67,
        68,  70,  71,  69,  71,  71,  154, 0,   0,   75,  242, 242, 243, 244, 0,   0,   0,   0,   0,   0,   0,   0,   0,   10 // Twelfth
    };

    bool IsEqual( const fheroes2::Image & one, const fheroes2::Image & two )
    {
        return one.width() == two.width() && one.height() == two.height();
    }

    bool Validate( const fheroes2::Image & in, uint32_t inX, uint32_t inY, const fheroes2::Image & out, uint32_t outX, uint32_t outY, uint32_t width, uint32_t height )
    {
        if ( in.empty() || out.empty() || width == 0 || height == 0 ) // what' the reason to work with empty images?
            return false;

        if ( inX > in.width() || inY > in.height() || outX > out.width() || outY > out.height() )
            return false;

        if ( ( inX + width > in.width() ) || ( inY + height > in.height() ) || ( outX + width > out.width() ) || ( outY + height > in.height() ) )
            return false;

        return true;
    }

    bool Verify( const fheroes2::Image & in, uint32_t & inX, uint32_t & inY, const fheroes2::Image & out, int32_t & outX, int32_t & outY, uint32_t & width,
                 uint32_t & height )
    {
        if ( in.empty() || out.empty() || width == 0 || height == 0 ) // what' the reason to work with empty images?
            return false;

        if ( inX > in.width() || inY > in.height() )
            return false;

        if ( outX < 0 ) {
            const uint32_t offsetX = static_cast<uint32_t>( -outX );
            if ( offsetX >= width )
                return false;

            inX += offsetX;
            outX = 0;
            width -= offsetX;
        }

        if ( outY < 0 ) {
            const uint32_t offsetY = static_cast<uint32_t>( -outY );
            if ( offsetY >= height )
                return false;

            inY += static_cast<uint32_t>( -outY );
            outY = 0;
            height -= offsetY;
        }

        if ( static_cast<uint32_t>( outX ) > out.width() || static_cast<uint32_t>( outY ) > out.height() )
            return false;

        if ( inX + width > in.width() ) {
            const uint32_t offsetX = ( inX + width ) - in.width();
            width -= offsetX;
        }

        if ( inY + height > in.height() ) {
            const uint32_t offsetY = ( inY + height ) - in.height();
            if ( offsetY >= height )
                return false;
            height -= offsetY;
        }

        if ( static_cast<uint32_t>( outX ) + width > out.width() ) {
            const uint32_t offsetX = ( static_cast<uint32_t>( outX ) + width ) - out.width();
            width -= offsetX;
        }

        if ( static_cast<uint32_t>( outY ) + height > out.height() ) {
            const uint32_t offsetY = ( static_cast<uint32_t>( outY ) + height ) - out.height();
            height -= offsetY;
        }

        return true;
    }
}

namespace fheroes2
{
    Image::Image( uint32_t width_, uint32_t height_ )
        : _width( 0u )
        , _height( 0u )
    {
        resize( width_, height_ );
    }

    Image::~Image()
    {
        clear();
    }

    uint32_t Image::width() const
    {
        return _width;
    }

    uint32_t Image::height() const
    {
        return _height;
    }

    uint8_t * Image::image()
    {
        return _image.data();
    }

    const uint8_t * Image::image() const
    {
        return _image.data();
    }

    uint8_t * Image::transform()
    {
        return _transform.data();
    }

    const uint8_t * Image::transform() const
    {
        return _transform.data();
    }

    void Image::clear()
    {
        _image.clear();
        _transform.clear();

        _width = 0u;
        _height = 0u;
    }

    void Image::fill( uint8_t value )
    {
        if ( !empty() ) {
            std::fill( _image.begin(), _image.end(), value );
        }
    }

    bool Image::empty() const
    {
        return _image.empty();
    }

    void Image::resize( uint32_t width_, uint32_t height_ )
    {
        if ( width_ == 0 || height_ == 0 || ( width_ == _width && height_ == _height ) ) // nothing to resize
            return;

        clear();

        _width = width_;
        _height = height_;

        _image.resize( _width * _height );
        _transform.resize( _width * _height );
    }

    void Image::reset()
    {
        if ( !empty() ) {
            std::fill( _image.begin(), _image.end(), 0u );
            std::fill( _transform.begin(), _transform.end(), 1u ); // skip all data
        }
    }

    Sprite::Sprite( uint32_t width_, uint32_t height_, int32_t x_, int32_t y_ )
        : Image( width_, height_ )
        , _x( x_ )
        , _y( y_ )
    {}

    Sprite::~Sprite() {}

    int32_t Sprite::x() const
    {
        return _x;
    }

    int32_t Sprite::y() const
    {
        return _y;
    }

    void Sprite::setPosition( int32_t x_, int32_t y_ )
    {
        _x = x_;
        _y = y_;
    }

    void ApplyPallete( Image & image, const std::vector<uint8_t> & palette )
    {
        ApplyPallete( image, image, palette );
    }

    void ApplyPallete( const Image & in, Image & out, const std::vector<uint8_t> & palette )
    {
        if ( !IsEqual( in, out ) || palette.size() != 256 ) {
            return;
        }

        const uint32_t width = in.width();
        const uint32_t height = in.height();

        const uint8_t * imageIn = in.image();
        const uint8_t * transformIn = in.transform();
        uint8_t * imageOut = out.image();
        const uint8_t * imageInEnd = imageIn + height * width;
        for ( ; imageIn != imageInEnd; ++imageIn, ++imageOut, ++transformIn ) {
            if ( *transformIn == 0 ) { // only modify pixels with data
                *imageOut = palette[*imageIn];
            }
        }
    }

    void Blit( const Image & in, Image & out, bool flip )
    {
        Blit( in, 0, 0, out, 0, 0, in.width(), in.height(), flip );
    }

    void Blit( const Image & in, Image & out, int32_t outX, int32_t outY, bool flip )
    {
        Blit( in, 0, 0, out, outX, outY, in.width(), in.height(), flip );
    }

    void Blit( const Image & in, uint32_t inX, uint32_t inY, Image & out, int32_t outX, int32_t outY, uint32_t width, uint32_t height, bool flip )
    {
        if ( !Verify( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        // Blitting one image onto another can be done only for image layer so we don't consider transform part of the output image
        const uint32_t widthIn = in.width();
        const uint32_t widthOut = out.width();

        if ( flip ) {
            const uint8_t * imageInY = in.image() + ( inY + height - 1 ) * widthIn + inX + width - 1;
            const uint8_t * transformY = in.transform() + ( inY + height - 1 ) * widthIn + inX + width - 1;
            uint8_t * imageOutY = out.image() + outY * out.width() + outX;
            const uint8_t * imageOutYEnd = imageOutY + height * widthOut;

            for ( ; imageOutY != imageOutYEnd; imageInY -= widthIn, transformY -= widthIn, imageOutY += widthOut ) {
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformX = transformY;
                uint8_t * imageOutX = imageOutY;
                const uint8_t * imageOutXEnd = imageOutX + width;

                for ( ; imageOutX != imageOutXEnd; --imageInX, --transformX, ++imageOutX ) {
                    if ( *transformX > 0 ) { // apply a tranformation
                        if ( *transformX > 1 ) { // 1 is to skip data
                            *imageOutX = *( transformTable + ( *transformX ) * 256 + *imageOutX );
                        }
                    }
                    else { // copy a pixel
                        *imageOutX = *imageInX;
                    }
                }
            }
        }
        else {
            const uint8_t * imageInY = in.image() + inY * widthIn + inX;
            const uint8_t * transformY = in.transform() + inY * widthIn + inX;
            uint8_t * imageOutY = out.image() + outY * out.width() + outX;
            const uint8_t * imageInYEnd = imageInY + height * widthIn;

            for ( ; imageInY != imageInYEnd; imageInY += widthIn, transformY += widthIn, imageOutY += widthOut ) {
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformX = transformY;
                uint8_t * imageOutX = imageOutY;
                const uint8_t * imageInXEnd = imageInX + width;

                for ( ; imageInX != imageInXEnd; ++imageInX, ++transformX, ++imageOutX ) {
                    if ( *transformX > 0 ) { // apply a tranformation
                        if ( *transformX > 1 ) { // 1 is to skip data
                            *imageOutX = *( transformTable + ( *transformX ) * 256 + *imageOutX );
                        }
                    }
                    else { // copy a pixel
                        *imageOutX = *imageInX;
                    }
                }
            }
        }
    }

    void Copy( const Image & in, Image & out )
    {
        out.resize( in.width(), in.height() );
        Copy( in, 0, 0, out, 0, 0, in.width(), in.height() );
    }

    void Copy( const Image & in, uint32_t inX, uint32_t inY, Image & out, uint32_t outX, uint32_t outY, uint32_t width, uint32_t height )
    {
        if ( !Validate( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        const uint32_t widthIn = in.width();
        const uint32_t widthOut = out.width();

        const uint8_t * imageInY = in.image() + inY * widthIn + inX;
        const uint8_t * transformInY = in.transform() + inY * widthIn + inX;
        uint8_t * imageOutY = out.image() + outY * out.width() + outX;
        uint8_t * transformOutY = out.transform() + outY * widthIn + outX;
        const uint8_t * imageInYEnd = imageInY + height * widthIn;

        for ( ; imageInY != imageInYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut, transformOutY += widthOut ) {
            memcpy( imageOutY, imageInY, width );
            memcpy( transformOutY, transformInY, width );
        }
    }

    Image CreateContour( const Image & image )
    {
        const uint32_t width = image.width();
        const uint32_t height = image.height();

        Image contour( width, height );
        if ( image.empty() || width < 2 || height < 2 ) {
            return contour;
        }

        const uint8_t * inY = image.transform();
        uint8_t * outY = contour.transform();

        const uint32_t reducedWidth = width - 1;
        const uint32_t reducedHeight = height - 1;

        for ( uint32_t y = 0; y < height; ++y, inY += width, outY += width ) {
            const uint8_t * inX = inY;

            const bool isTopRow = ( y == 0 );
            const bool isBottomRow = ( y == reducedHeight );

            for ( uint32_t x = 0; x < width; ++x, ++inX ) {
                if ( *inX > 0 ) { // empty or shadow
                    if ( ( x > 0 && *( inX - 1 ) == 0 ) || ( x < reducedWidth && *( inX + 1 ) == 0 ) || ( !isTopRow && *( inX - width ) == 0 )
                         || ( !isBottomRow && *( inX + width ) == 0 ) ) {
                        outY[x] = 0;
                    }
                }
            }
        }

        return contour;
    }

    Sprite Crop( const Image & image, int32_t x, int32_t y, uint32_t width, uint32_t height )
    {
        if ( image.empty() || width == 0 || height == 0 )
            return Sprite();

        if ( x < 0 ) {
            const uint32_t offsetX = static_cast<uint32_t>( -x );
            if ( offsetX >= width )
                return Sprite();

            x = 0;
            width -= offsetX;
        }

        if ( y < 0 ) {
            const uint32_t offsetY = static_cast<uint32_t>( -y );
            if ( offsetY >= height )
                return Sprite();

            y = 0;
            height -= offsetY;
        }

        if ( static_cast<uint32_t>( x ) > image.width() || static_cast<uint32_t>( y ) > image.height() )
            return Sprite();

        if ( static_cast<uint32_t>( x ) + width > image.width() ) {
            const uint32_t offsetX = ( static_cast<uint32_t>( x ) + width ) - image.width();
            width -= offsetX;
        }

        if ( static_cast<uint32_t>( y ) + height > image.height() ) {
            const uint32_t offsetY = ( static_cast<uint32_t>( y ) + height ) - image.height();
            height -= offsetY;
        }

        Sprite out( width, height );
        Copy( image, static_cast<uint32_t>( x ), static_cast<uint32_t>( y ), out, 0, 0, width, height );
        out.setPosition( x, y );
        return out;
    }

    uint8_t GetColorId( uint8_t red, uint8_t green, uint8_t blue )
    {
        double minDistance = sqrt( 3 * 255 * 255 );
        uint32_t bestPos = 0;

        for ( uint32_t i = 0; i < 256; ++i ) {
            const double offsetRed = static_cast<double>( kb_pal[i * 3] ) - static_cast<double>( red );
            const double offsetGreen = static_cast<double>( kb_pal[i * 3 + 1] ) - static_cast<double>( green );
            const double offsetBlue = static_cast<double>( kb_pal[i * 3 + 2] ) - static_cast<double>( blue );
            const double distance = sqrt( offsetRed * offsetRed + offsetGreen * offsetGreen + offsetBlue * offsetBlue );
            if ( minDistance > distance ) {
                minDistance = distance;
                bestPos = i;
            }
        }

        return static_cast<uint8_t>( bestPos ); // it's safe to cast
    }

    void Resize( const Image & in, Image & out )
    {
        if ( in.empty() || out.empty() ) {
            return;
        }

        if ( IsEqual( in, out ) ) {
            Copy( in, out );
            return;
        }

        const uint32_t widthIn = in.width();
        const uint32_t heightIn = in.height();
        const uint32_t widthOut = out.width();
        const uint32_t heightOut = out.height();

        const uint8_t * inY = in.image();
        const uint8_t * transformInY = in.transform();
        uint8_t * outY = out.image();
        uint8_t * transformOutY = out.transform();
        const uint8_t * outYEnd = outY + widthOut * heightOut;

        uint32_t idY = 0;

        // Precalculation of X position
        std::vector<uint32_t> positionX( widthOut );
        for ( uint32_t x = 0; x < widthOut; ++x )
            positionX[x] = x * widthIn / widthOut;

        for ( ; outY != outYEnd; outY += widthOut, transformOutY += widthOut, ++idY ) {
            uint8_t * outX = outY;
            uint8_t * transformOutX = transformOutY;
            const uint8_t * outXEnd = outX + widthOut;

            const uint32_t offset = ( idY * heightIn / heightOut ) * widthIn;
            const uint8_t * inX = inY + offset;
            const uint8_t * transformInX = transformInY + offset;
            const uint32_t * idX = positionX.data();

            for ( ; outX != outXEnd; ++outX, ++idX ) {
                *outX = *( inX + ( *idX ) );
                *transformOutX = *( transformInX + ( *idX ) );
            }
        }
    }

    bool SaveBitmap( const Image & image, const std::string & path )
    {
        const uint32_t width = image.width();
        const uint32_t height = image.height();

        FILE * fp = fopen( path.c_str(), "wb" );
        if ( !fp ) {
            return false;
        }

        fwrite( "BM", 2, 1, fp );
        uint32_t temp = 1078 + ( width * height );
        fwrite( &temp, 4, 1, fp );
        temp = 0;
        fwrite( &temp, 4, 1, fp );
        temp = 1078;
        fwrite( &temp, 4, 1, fp );
        temp = 40;
        fwrite( &temp, 4, 1, fp );
        fwrite( &width, 4, 1, fp );
        fwrite( &height, 4, 1, fp );
        temp = 1;
        fwrite( &temp, 2, 1, fp );
        temp = 8;
        fwrite( &temp, 4, 1, fp );
        temp = 0;
        fwrite( &temp, 2, 1, fp );
        temp = width * height;
        fwrite( &temp, 4, 1, fp );
        temp = 0;
        fwrite( &temp, 4, 1, fp );
        fwrite( &temp, 4, 1, fp );
        temp = 256;
        fwrite( &temp, 4, 1, fp );
        temp = 256;
        fwrite( &temp, 4, 1, fp );
        temp = 0;
        for ( uint32_t i = 0; i < 256; ++i ) {
            uint8_t value = kb_pal[i * 3 + 2] * 4;
            fwrite( &value, 1, 1, fp );
            value = kb_pal[i * 3 + 1] * 4;
            fwrite( &value, 1, 1, fp );
            value = kb_pal[i * 3] * 4;
            fwrite( &value, 1, 1, fp );
            fwrite( &temp, 1, 1, fp );
        }

        uint32_t extraBytes = width % 4;
        if ( extraBytes > 0 )
            extraBytes = 4 - extraBytes;

        uint8_t extraData[4] = {0};

        for ( int i = height - 1; i >= 0; --i ) {
            fwrite( image.image() + i * width, width, 1, fp );
            if ( extraBytes > 0 ) {
                fwrite( extraData, extraBytes, 1, fp );
            }
        }

        fclose( fp );
        return true;
    }
}
