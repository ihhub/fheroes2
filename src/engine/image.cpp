/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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
#include "image_palette.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace
{
    // 0 in shadow part means no shadow, 1 means skip any drawings so to don't waste extra CPU cycles for ( tableId - 2 ) command we just add extra fake tables
    // Mirror palette was modified as it was containing 238, 238, 239, 240 values instead of 238, 239, 240, 241
    const uint8_t transformTable[256 * 16] = {
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
        68,  70,  71,  69,  71,  71,  154, 0,   0,   75,  242, 242, 243, 244, 0,   0,   0,   0,   0,   0,   0,   0,   0,   10, // Twelfth

        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  10,  10,  10,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
        25,  26,  27,  28,  29,  30,  31,  32,  37,  37,  37,  37,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
        54,  55,  56,  57,  58,  63,  63,  63,  63,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  85,  85,
        85,  85,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 108, 108, 108, 108, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 131, 131, 131, 131, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
        141, 142, 143, 144, 145, 146, 147, 152, 152, 152, 152, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
        170, 175, 175, 175, 175, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 198, 198, 198, 198, 198,
        199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 214, 215, 216, 217, 218, 219, 220, 221, 222, 222, 223, 224, 225, 226, 227, 228, 229, 231,
        232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 242, 243, 244, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, // Mirror

        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
        29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,
        58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,
        87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
        116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
        145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173,
        174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
        203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 188, 188, 188, 188, 118, 118, 118, 118, 222, 223, 224, 225, 226, 227, 228, 229, 230, 69,
        69,  69,  69,  69,  69,  69,  69,  69,  69,  69,  242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255 // No cycle
    };

    bool Validate( const fheroes2::Image & image, int32_t x, int32_t y, int32_t width, int32_t height )
    {
        if ( image.empty() || width <= 0 || height <= 0 ) // what's the reason to work with empty images?
            return false;

        if ( x < 0 || y < 0 || x + width > image.width() || y + height > image.height() )
            return false;

        return true;
    }

    bool Verify( const fheroes2::Image & image, int32_t & x, int32_t & y, int32_t & width, int32_t & height )
    {
        if ( image.empty() || width <= 0 || height <= 0 ) // what's the reason to work with empty images?
            return false;

        const int32_t widthOut = image.width();
        const int32_t heightOut = image.height();

        if ( x < 0 ) {
            const int32_t offsetX = -x;
            if ( offsetX >= width )
                return false;

            x = 0;
            width -= offsetX;
        }

        if ( y < 0 ) {
            const int32_t offsetY = -y;
            if ( offsetY >= height )
                return false;

            y = 0;
            height -= offsetY;
        }

        if ( x > widthOut || y > heightOut )
            return false;

        if ( x + width > widthOut ) {
            const int32_t offsetX = x + width - widthOut;
            if ( offsetX >= width )
                return false;
            width -= offsetX;
        }

        if ( y + height > heightOut ) {
            const int32_t offsetY = y + height - heightOut;
            if ( offsetY >= height )
                return false;
            height -= offsetY;
        }

        return true;
    }

    bool Verify( int32_t & inX, int32_t & inY, int32_t & outX, int32_t & outY, int32_t & width, int32_t & height, int32_t widthIn, int32_t heightIn, int32_t widthOut,
                 int32_t heightOut )
    {
        if ( widthIn <= 0 || heightIn <= 0 || widthOut <= 0 || heightOut <= 0 || width <= 0 || height <= 0 ) // what's the reason to work with empty images?
            return false;

        if ( inX < 0 || inY < 0 || inX > widthIn || inY > heightIn )
            return false;

        if ( outX < 0 ) {
            const int32_t offsetX = -outX;
            if ( offsetX >= width )
                return false;

            inX += offsetX;
            outX = 0;
            width -= offsetX;
        }

        if ( outY < 0 ) {
            const int32_t offsetY = -outY;
            if ( offsetY >= height )
                return false;

            inY += offsetY;
            outY = 0;
            height -= offsetY;
        }

        if ( outX > widthOut || outY > heightOut )
            return false;

        if ( inX + width > widthIn ) {
            const int32_t offsetX = inX + width - widthIn;
            if ( offsetX >= width )
                return false;
            width -= offsetX;
        }

        if ( inY + height > heightIn ) {
            const int32_t offsetY = inY + height - heightIn;
            if ( offsetY >= height )
                return false;
            height -= offsetY;
        }

        if ( outX + width > widthOut ) {
            const int32_t offsetX = outX + width - widthOut;
            if ( offsetX >= width )
                return false;
            width -= offsetX;
        }

        if ( outY + height > heightOut ) {
            const int32_t offsetY = outY + height - heightOut;
            if ( offsetY >= height )
                return false;
            height -= offsetY;
        }

        return true;
    }

    bool Verify( const fheroes2::Image & in, int32_t & inX, int32_t & inY, const fheroes2::Image & out, int32_t & outX, int32_t & outY, int32_t & width,
                 int32_t & height )
    {
        return Verify( inX, inY, outX, outY, width, height, in.width(), in.height(), out.width(), out.height() );
    }

    uint8_t GetPALColorId( uint8_t red, uint8_t green, uint8_t blue )
    {
        static uint8_t rgbToId[64 * 64 * 64];
        static bool isInitialized = false;
        if ( !isInitialized ) {
            isInitialized = true;
            const uint32_t size = 64 * 64 * 64;

            uint32_t r = 0;
            uint32_t g = 0;
            uint32_t b = 0;

            const uint8_t * gamePalette = fheroes2::getGamePalette();

            for ( uint32_t id = 0; id < size; ++id ) {
                r = ( id % 64 );
                g = ( id >> 6 ) % 64;
                b = ( id >> 12 );
                int32_t minDistance = 3 * 255 * 255;
                uint32_t bestPos = 0;

                const uint8_t * correctorX = transformTable + 256 * 15;

                for ( uint32_t i = 0; i < 256; ++i, ++correctorX ) {
                    const uint8_t * palette = gamePalette + *correctorX * 3;

                    const int32_t offsetRed = static_cast<int32_t>( *palette ) - static_cast<int32_t>( r );
                    ++palette;
                    const int32_t offsetGreen = static_cast<int32_t>( *palette ) - static_cast<int32_t>( g );
                    ++palette;
                    const int32_t offsetBlue = static_cast<int32_t>( *palette ) - static_cast<int32_t>( b );
                    ++palette;
                    const int32_t distance = offsetRed * offsetRed + offsetGreen * offsetGreen + offsetBlue * offsetBlue;
                    if ( minDistance > distance ) {
                        minDistance = distance;
                        bestPos = *correctorX;
                    }
                }

                rgbToId[id] = static_cast<uint8_t>( bestPos ); // it's safe to cast
            }
        }

        return rgbToId[red + green * 64 + blue * 64 * 64];
    }

    void ApplyRawPalette( const fheroes2::Image & in, int32_t inX, int32_t inY, fheroes2::Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height,
                          const uint8_t * palette )
    {
        if ( !Verify( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();

        const uint8_t * imageInY = in.image() + inY * widthIn + inX;
        uint8_t * imageOutY = out.image() + outY * widthOut + outX;
        const uint8_t * imageInYEnd = imageInY + height * widthIn;

        if ( in.singleLayer() ) {
            // All pixels in a single layer image do not have any transform values so there is no need to check for them.
            for ( ; imageInY != imageInYEnd; imageInY += widthIn, imageOutY += widthOut ) {
                const uint8_t * imageInX = imageInY;
                uint8_t * imageOutX = imageOutY;
                const uint8_t * imageInXEnd = imageInX + width;

                for ( ; imageInX != imageInXEnd; ++imageInX, ++imageOutX ) {
                    *imageOutX = palette[*imageInX];
                }
            }
        }
        else {
            const uint8_t * transformInY = in.transform() + inY * widthIn + inX;

            for ( ; imageInY != imageInYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut ) {
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformInX = transformInY;
                uint8_t * imageOutX = imageOutY;
                const uint8_t * imageInXEnd = imageInX + width;

                for ( ; imageInX != imageInXEnd; ++imageInX, ++imageOutX, ++transformInX ) {
                    if ( *transformInX == 0 ) { // only modify pixels with data
                        *imageOutX = palette[*imageInX];
                    }
                }
            }
        }
    }
}

namespace fheroes2
{
    Image::Image()
        : _width( 0 )
        , _height( 0 )
        , _singleLayer( false )
    {
        // Do nothing.
    }

    Image::Image( int32_t width_, int32_t height_ )
        : _width( 0 )
        , _height( 0 )
        , _singleLayer( false )
    {
        Image::resize( width_, height_ );
    }

    Image::Image( const Image & image_ )
        : _width( 0 )
        , _height( 0 )
        , _singleLayer( false )
    {
        copy( image_ );
    }

    Image::Image( Image && image_ ) noexcept
        : _width( 0 )
        , _height( 0 )
        , _data( std::move( image_._data ) )
        , _singleLayer( false )
    {
        std::swap( _singleLayer, image_._singleLayer );
        std::swap( _width, image_._width );
        std::swap( _height, image_._height );
    }

    Image & Image::operator=( const Image & image_ )
    {
        if ( this != &image_ ) {
            copy( image_ );
        }

        return *this;
    }

    Image & Image::operator=( Image && image_ ) noexcept
    {
        if ( this != &image_ ) {
            // We shouldn't copy or move different types of images.
            assert( _singleLayer == image_._singleLayer );

            std::swap( _width, image_._width );
            std::swap( _height, image_._height );
            std::swap( _data, image_._data );
        }

        return *this;
    }

    uint8_t * Image::image()
    {
        return _data.get();
    }

    const uint8_t * Image::image() const
    {
        return _data.get();
    }

    void Image::clear()
    {
        _data.reset();

        _width = 0;
        _height = 0;
    }

    void Image::fill( uint8_t value )
    {
        if ( !empty() ) {
            const size_t totalSize = static_cast<size_t>( _width * _height );
            std::fill( image(), image() + totalSize, value );
            std::fill( transform(), transform() + totalSize, static_cast<uint8_t>( 0 ) );
        }
    }

    void Image::resize( int32_t width_, int32_t height_ )
    {
        if ( width_ == _width && height_ == _height ) {
            return;
        }

        if ( width_ <= 0 || height_ <= 0 ) {
            clear();

            return;
        }

        const size_t size = static_cast<size_t>( width_ * height_ );

        _data.reset( new uint8_t[size * 2] );

        _width = width_;
        _height = height_;
    }

    void Image::reset()
    {
        if ( !empty() ) {
            const size_t totalSize = static_cast<size_t>( _width * _height );
            std::fill( image(), image() + totalSize, static_cast<uint8_t>( 0 ) );
            std::fill( transform(), transform() + totalSize, static_cast<uint8_t>( 1 ) ); // skip all data
        }
    }

    void Image::copy( const Image & image )
    {
        // We shouldn't copy or move different types of images.
        assert( _singleLayer == image._singleLayer );

        if ( !image._data ) {
            clear();

            return;
        }

        const size_t size = static_cast<size_t>( image._width * image._height );

        if ( image._width != _width || image._height != _height ) {
            _data.reset( new uint8_t[size * 2] );

            _width = image._width;
            _height = image._height;
        }

        memcpy( _data.get(), image._data.get(), size * 2 );
    }

    Sprite::Sprite()
        : Image( 0, 0 )
        , _x( 0 )
        , _y( 0 )
    {
        // Do nothing.
    }

    Sprite::Sprite( int32_t width_, int32_t height_, int32_t x_, int32_t y_ )
        : Image( width_, height_ )
        , _x( x_ )
        , _y( y_ )
    {
        // Do nothing.
    }

    Sprite::Sprite( const Image & image, int32_t x_, int32_t y_ )
        : Image( image )
        , _x( x_ )
        , _y( y_ )
    {
        // Do nothing.
    }

    Sprite::Sprite( const Sprite & sprite )
        : Image( sprite )
        , _x( sprite._x )
        , _y( sprite._y )
    {
        // Do nothing.
    }

    Sprite::Sprite( Sprite && sprite ) noexcept
        : Image( std::move( sprite ) )
        , _x( 0 )
        , _y( 0 )
    {
        std::swap( _x, sprite._x );
        std::swap( _y, sprite._y );
    }

    Sprite & Sprite::operator=( const Sprite & sprite )
    {
        if ( this != &sprite ) {
            Image::operator=( sprite );

            _x = sprite._x;
            _y = sprite._y;
        }

        return *this;
    }

    Sprite & Sprite::operator=( Sprite && sprite ) noexcept
    {
        if ( this != &sprite ) {
            Image::operator=( std::move( sprite ) );

            std::swap( _x, sprite._x );
            std::swap( _y, sprite._y );
        }

        return *this;
    }

    void Sprite::setPosition( int32_t x_, int32_t y_ )
    {
        _x = x_;
        _y = y_;
    }

    ImageRestorer::ImageRestorer( Image & image )
        : _image( image )
        , _x( 0 )
        , _y( 0 )
        , _width( image.width() )
        , _height( image.height() )
        , _isRestored( false )
    {
        _updateRoi();
        _copy.resize( _width, _height );
        if ( _image.singleLayer() ) {
            _copy._disableTransformLayer();
        }

        Copy( _image, _x, _y, _copy, 0, 0, _width, _height );
    }

    ImageRestorer::ImageRestorer( Image & image, int32_t x_, int32_t y_, int32_t width, int32_t height )
        : _image( image )
        , _x( x_ )
        , _y( y_ )
        , _width( width )
        , _height( height )
        , _isRestored( false )
    {
        _updateRoi();
        _copy.resize( _width, _height );
        if ( _image.singleLayer() ) {
            _copy._disableTransformLayer();
        }

        Copy( _image, _x, _y, _copy, 0, 0, _width, _height );
    }

    ImageRestorer::~ImageRestorer()
    {
        if ( !_isRestored ) {
            restore();
        }
    }

    void ImageRestorer::update( int32_t x_, int32_t y_, int32_t width, int32_t height )
    {
        _isRestored = false;
        _x = x_;
        _y = y_;
        _width = width;
        _height = height;
        _updateRoi();

        _copy.resize( _width, _height );
        Copy( _image, _x, _y, _copy, 0, 0, _width, _height );
    }

    void ImageRestorer::restore()
    {
        _isRestored = true;
        Copy( _copy, 0, 0, _image, _x, _y, _width, _height );
    }

    void ImageRestorer::reset()
    {
        _isRestored = true;
    }

    void ImageRestorer::_updateRoi()
    {
        if ( _width < 0 )
            _width = 0;

        if ( _height < 0 )
            _height = 0;

        if ( _x < 0 ) {
            const int32_t offset = -_x;
            _x = 0;
            _width = _width < offset ? 0 : _width - offset;
        }

        if ( _y < 0 ) {
            const int32_t offset = -_y;
            _y = 0;
            _height = _height < offset ? 0 : _height - offset;
        }

        if ( _x >= _image.width() || _y >= _image.height() ) {
            _x = 0;
            _y = 0;
            _width = 0;
            _height = 0;
            return;
        }

        if ( _x + _width > _image.width() ) {
            const int32_t offsetX = _x + _width - _image.width();
            if ( offsetX >= _width ) {
                _x = 0;
                _y = 0;
                _width = 0;
                _height = 0;
                return;
            }
            _width -= offsetX;
        }

        if ( _y + _height > _image.height() ) {
            const int32_t offsetY = _y + _height - _image.height();
            if ( offsetY >= _height ) {
                _x = 0;
                _y = 0;
                _width = 0;
                _height = 0;
                return;
            }
            _height -= offsetY;
        }
    }

    Sprite addShadow( const Sprite & in, const Point & shadowOffset, const uint8_t transformId )
    {
        if ( in.empty() || shadowOffset.x > 0 || shadowOffset.y < 0 )
            return in;

        Sprite out = makeShadow( in, shadowOffset, transformId );
        Blit( in, out, -shadowOffset.x, 0 );

        return out;
    }

    void AddTransparency( Image & image, uint8_t valueToReplace )
    {
        ReplaceColorIdByTransformId( image, valueToReplace, 1 );
    }

    void AlphaBlit( const Image & in, Image & out, uint8_t alphaValue, bool flip )
    {
        AlphaBlit( in, 0, 0, out, 0, 0, in.width(), in.height(), alphaValue, flip );
    }

    void AlphaBlit( const Image & in, Image & out, int32_t outX, int32_t outY, uint8_t alphaValue, bool flip )
    {
        AlphaBlit( in, 0, 0, out, outX, outY, in.width(), in.height(), alphaValue, flip );
    }

    void AlphaBlit( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t alphaValue, bool flip )
    {
        if ( alphaValue == 0 ) { // there is nothing we need to do
            return;
        }

        if ( alphaValue == 255 ) {
            Blit( in, inX, inY, out, outX, outY, width, height, flip );
            return;
        }

        if ( !Verify( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        // Blitting one image onto another can be done only for image layer so we don't consider transform part of the output image
        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();

        const uint8_t behindValue = 255 - alphaValue;

        const uint8_t * gamePalette = getGamePalette();

        if ( flip ) {
            const int32_t offsetInY = inY * widthIn + widthIn - 1 - inX;
            const uint8_t * imageInY = in.image() + offsetInY;
            const uint8_t * transformInY = in.transform() + offsetInY;

            const int32_t offsetOutY = outY * widthOut + outX;
            uint8_t * imageOutY = out.image() + offsetOutY;
            const uint8_t * imageOutYEnd = imageOutY + height * widthOut;

            for ( ; imageOutY != imageOutYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut ) {
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformInX = transformInY;
                uint8_t * imageOutX = imageOutY;
                const uint8_t * imageOutXEnd = imageOutX + width;

                for ( ; imageOutX != imageOutXEnd; --imageInX, --transformInX, ++imageOutX ) {
                    if ( *transformInX == 1 ) { // skip pixel
                        continue;
                    }

                    uint8_t inValue = *imageInX;
                    if ( *transformInX > 1 ) {
                        inValue = *( transformTable + ( *transformInX ) * 256 + *imageOutX );
                    }

                    const uint8_t * inPAL = gamePalette + inValue * 3;
                    const uint8_t * outPAL = gamePalette + ( *imageOutX ) * 3;

                    const uint32_t red = static_cast<uint32_t>( *inPAL ) * alphaValue + static_cast<uint32_t>( *outPAL ) * behindValue;
                    const uint32_t green = static_cast<uint32_t>( *( inPAL + 1 ) ) * alphaValue + static_cast<uint32_t>( *( outPAL + 1 ) ) * behindValue;
                    const uint32_t blue = static_cast<uint32_t>( *( inPAL + 2 ) ) * alphaValue + static_cast<uint32_t>( *( outPAL + 2 ) ) * behindValue;
                    *imageOutX = GetPALColorId( static_cast<uint8_t>( red / 255 ), static_cast<uint8_t>( green / 255 ), static_cast<uint8_t>( blue / 255 ) );
                }
            }
        }
        else {
            const int32_t offsetInY = inY * widthIn + inX;
            const uint8_t * imageInY = in.image() + offsetInY;
            const uint8_t * transformInY = in.transform() + offsetInY;

            uint8_t * imageOutY = out.image() + outY * widthOut + outX;
            const uint8_t * imageInYEnd = imageInY + height * widthIn;

            for ( ; imageInY != imageInYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut ) {
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformInX = transformInY;
                uint8_t * imageOutX = imageOutY;
                const uint8_t * imageInXEnd = imageInX + width;

                for ( ; imageInX != imageInXEnd; ++imageInX, ++transformInX, ++imageOutX ) {
                    if ( *transformInX == 1 ) { // skip pixel
                        continue;
                    }

                    uint8_t inValue = *imageInX;
                    if ( *transformInX > 1 ) {
                        inValue = *( transformTable + ( *transformInX ) * 256 + *imageOutX );
                    }

                    const uint8_t * inPAL = gamePalette + inValue * 3;
                    const uint8_t * outPAL = gamePalette + ( *imageOutX ) * 3;

                    const uint32_t red = static_cast<uint32_t>( *inPAL ) * alphaValue + static_cast<uint32_t>( *outPAL ) * behindValue;
                    const uint32_t green = static_cast<uint32_t>( *( inPAL + 1 ) ) * alphaValue + static_cast<uint32_t>( *( outPAL + 1 ) ) * behindValue;
                    const uint32_t blue = static_cast<uint32_t>( *( inPAL + 2 ) ) * alphaValue + static_cast<uint32_t>( *( outPAL + 2 ) ) * behindValue;
                    *imageOutX = GetPALColorId( static_cast<uint8_t>( red / 255 ), static_cast<uint8_t>( green / 255 ), static_cast<uint8_t>( blue / 255 ) );
                }
            }
        }
    }

    void ApplyPalette( Image & image, const std::vector<uint8_t> & palette )
    {
        ApplyPalette( image, image, palette );
    }

    void ApplyPalette( const Image & in, Image & out, const std::vector<uint8_t> & palette )
    {
        if ( palette.size() != 256 ) {
            return;
        }

        ApplyRawPalette( in, 0, 0, out, 0, 0, in.width(), in.height(), palette.data() );
    }

    void ApplyPalette( Image & image, uint8_t paletteId )
    {
        ApplyPalette( image, image, paletteId );
    }

    void ApplyPalette( const Image & in, Image & out, uint8_t paletteId )
    {
        if ( paletteId > 15 ) {
            return;
        }

        ApplyRawPalette( in, 0, 0, out, 0, 0, in.width(), in.height(), transformTable + paletteId * 256 );
    }

    void ApplyPalette( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t paletteId )
    {
        if ( paletteId > 15 ) {
            return;
        }

        ApplyRawPalette( in, inX, inY, out, outX, outY, width, height, transformTable + paletteId * 256 );
    }

    void ApplyPalette( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height,
                       const std::vector<uint8_t> & palette )
    {
        if ( palette.size() != 256 ) {
            return;
        }

        ApplyRawPalette( in, inX, inY, out, outX, outY, width, height, palette.data() );
    }

    void ApplyAlpha( const Image & in, Image & out, uint8_t alpha )
    {
        ApplyAlpha( in, 0, 0, out, 0, 0, in.width(), in.height(), alpha );
    }

    void ApplyAlpha( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t alpha )
    {
        std::vector<uint8_t> palette( 256 );

        const uint8_t * value = getGamePalette();

        for ( uint32_t i = 0; i < 256; ++i ) {
            const uint32_t red = static_cast<uint32_t>( *value ) * alpha / 255;
            ++value;
            const uint32_t green = static_cast<uint32_t>( *value ) * alpha / 255;
            ++value;
            const uint32_t blue = static_cast<uint32_t>( *value ) * alpha / 255;
            ++value;
            palette[i] = GetPALColorId( static_cast<uint8_t>( red ), static_cast<uint8_t>( green ), static_cast<uint8_t>( blue ) );
        }

        ApplyPalette( in, inX, inY, out, outX, outY, width, height, palette );
    }

    void ApplyTransform( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t transformId )
    {
        if ( !Verify( image, x, y, width, height ) )
            return;

        const int32_t imageWidth = image.width();

        uint8_t * imageY = image.image() + y * imageWidth + x;
        const uint8_t * imageYEnd = imageY + height * imageWidth;

        if ( image.singleLayer() ) {
            for ( ; imageY != imageYEnd; imageY += imageWidth ) {
                uint8_t * imageX = imageY;
                const uint8_t * imageXEnd = imageX + width;

                for ( ; imageX != imageXEnd; ++imageX ) {
                    *imageX = *( transformTable + transformId * 256 + *imageX );
                }
            }
        }
        else {
            const uint8_t * transformY = image.transform() + y * imageWidth + x;

            for ( ; imageY != imageYEnd; imageY += imageWidth, transformY += imageWidth ) {
                uint8_t * imageX = imageY;
                const uint8_t * transformX = transformY;
                const uint8_t * imageXEnd = imageX + width;

                for ( ; imageX != imageXEnd; ++imageX, ++transformX ) {
                    if ( *transformX == 0 ) {
                        *imageX = *( transformTable + transformId * 256 + *imageX );
                    }
                }
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

    void Blit( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, bool flip )
    {
        if ( in.singleLayer() && out.singleLayer() && !flip ) {
            Copy( in, inX, inY, out, outX, outY, width, height );
            return;
        }

        if ( !Verify( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();

        if ( flip ) {
            const int32_t offsetInY = inY * widthIn + widthIn - 1 - inX;
            const uint8_t * imageInY = in.image() + offsetInY;
            const uint8_t * transformInY = in.transform() + offsetInY;

            const int32_t offsetOutY = outY * widthOut + outX;
            uint8_t * imageOutY = out.image() + offsetOutY;
            const uint8_t * imageOutYEnd = imageOutY + height * widthOut;

            if ( out.singleLayer() ) {
                assert( !in.singleLayer() );
                for ( ; imageOutY != imageOutYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut ) {
                    const uint8_t * imageInX = imageInY;
                    const uint8_t * transformInX = transformInY;
                    uint8_t * imageOutX = imageOutY;
                    const uint8_t * imageOutXEnd = imageOutX + width;

                    for ( ; imageOutX != imageOutXEnd; --imageInX, --transformInX, ++imageOutX ) {
                        if ( *transformInX > 0 ) { // apply a transformation
                            if ( *transformInX != 1 ) { // skip pixel
                                *imageOutX = *( transformTable + ( *transformInX ) * 256 + *imageOutX );
                            }
                        }
                        else { // copy a pixel
                            *imageOutX = *imageInX;
                        }
                    }
                }
            }
            else {
                uint8_t * transformOutY = out.transform() + offsetOutY;

                for ( ; imageOutY != imageOutYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut, transformOutY += widthOut ) {
                    const uint8_t * imageInX = imageInY;
                    const uint8_t * transformInX = transformInY;
                    uint8_t * imageOutX = imageOutY;
                    uint8_t * transformOutX = transformOutY;
                    const uint8_t * imageOutXEnd = imageOutX + width;

                    for ( ; imageOutX != imageOutXEnd; --imageInX, --transformInX, ++imageOutX, ++transformOutX ) {
                        if ( *transformInX == 1 ) { // skip pixel
                            continue;
                        }

                        if ( *transformInX > 0 && *transformOutX == 0 ) { // apply a transformation
                            *imageOutX = *( transformTable + ( *transformInX ) * 256 + *imageOutX );
                        }
                        else { // copy a pixel
                            *transformOutX = *transformInX;
                            *imageOutX = *imageInX;
                        }
                    }
                }
            }
        }
        else {
            const int32_t offsetInY = inY * widthIn + inX;
            const uint8_t * imageInY = in.image() + offsetInY;
            const uint8_t * transformInY = in.transform() + offsetInY;

            const int32_t offsetOutY = outY * widthOut + outX;
            uint8_t * imageOutY = out.image() + offsetOutY;
            const uint8_t * imageInYEnd = imageInY + height * widthIn;

            if ( out.singleLayer() ) {
                assert( !in.singleLayer() );
                for ( ; imageInY != imageInYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut ) {
                    const uint8_t * imageInX = imageInY;
                    const uint8_t * transformInX = transformInY;
                    uint8_t * imageOutX = imageOutY;
                    const uint8_t * imageInXEnd = imageInX + width;

                    for ( ; imageInX != imageInXEnd; ++imageInX, ++transformInX, ++imageOutX ) {
                        if ( *transformInX > 0 ) { // apply a transformation
                            if ( *transformInX != 1 ) { // skip pixel
                                *imageOutX = *( transformTable + ( *transformInX ) * 256 + *imageOutX );
                            }
                        }
                        else { // copy a pixel
                            *imageOutX = *imageInX;
                        }
                    }
                }
            }
            else {
                uint8_t * transformOutY = out.transform() + offsetOutY;

                for ( ; imageInY != imageInYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut, transformOutY += widthOut ) {
                    const uint8_t * imageInX = imageInY;
                    const uint8_t * transformInX = transformInY;
                    uint8_t * imageOutX = imageOutY;
                    uint8_t * transformOutX = transformOutY;
                    const uint8_t * imageInXEnd = imageInX + width;

                    for ( ; imageInX != imageInXEnd; ++imageInX, ++transformInX, ++imageOutX, ++transformOutX ) {
                        if ( *transformInX == 1 ) { // skip pixel
                            continue;
                        }

                        if ( *transformInX > 0 && *transformOutX == 0 ) { // apply a transformation
                            *imageOutX = *( transformTable + ( *transformInX ) * 256 + *imageOutX );
                        }
                        else { // copy a pixel
                            *transformOutX = *transformInX;
                            *imageOutX = *imageInX;
                        }
                    }
                }
            }
        }
    }

    void Blit( const Image & in, const Point & inPos, Image & out, const Point & outPos, const Size & size, bool flip )
    {
        if ( inPos.x < 0 || inPos.y < 0 )
            return;

        Blit( in, inPos.x, inPos.y, out, outPos.x, outPos.y, size.width, size.height, flip );
    }

    void Copy( const Image & in, Image & out )
    {
        out.resize( in.width(), in.height() );
        Copy( in, 0, 0, out, 0, 0, in.width(), in.height() );
    }

    void Copy( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height )
    {
        if ( !Verify( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();

        const int32_t offsetInY = inY * widthIn + inX;
        const uint8_t * imageInY = in.image() + offsetInY;

        const int32_t offsetOutY = outY * widthOut + outX;
        uint8_t * imageOutY = out.image() + offsetOutY;
        const uint8_t * imageOutYEnd = imageOutY + height * widthOut;

        if ( out.singleLayer() ) {
            for ( ; imageOutY != imageOutYEnd; imageInY += widthIn, imageOutY += widthOut ) {
                memcpy( imageOutY, imageInY, static_cast<size_t>( width ) );
            }
        }
        else if ( in.singleLayer() ) {
            uint8_t * transformOutY = out.transform() + offsetOutY;

            for ( ; imageOutY != imageOutYEnd; imageInY += widthIn, imageOutY += widthOut, transformOutY += widthOut ) {
                memcpy( imageOutY, imageInY, static_cast<size_t>( width ) );
                std::fill( transformOutY, transformOutY + width, static_cast<uint8_t>( 0 ) );
            }
        }
        else {
            const uint8_t * transformInY = in.transform() + offsetInY;
            uint8_t * transformOutY = out.transform() + offsetOutY;

            for ( ; imageOutY != imageOutYEnd; imageInY += widthIn, transformInY += widthIn, imageOutY += widthOut, transformOutY += widthOut ) {
                memcpy( imageOutY, imageInY, static_cast<size_t>( width ) );
                memcpy( transformOutY, transformInY, static_cast<size_t>( width ) );
            }
        }
    }

    void CopyTransformLayer( const Image & in, Image & out )
    {
        if ( in.empty() || out.empty() || in.singleLayer() || out.singleLayer() || in.width() != out.width() || in.height() != out.height() ) {
            assert( 0 );
            return;
        }
        memcpy( out.transform(), in.transform(), in.width() * in.height() );
    }

    Image CreateBlurredImage( const Image & in, int32_t blurRadius )
    {
        if ( in.empty() )
            return Image();

        if ( blurRadius < 1 )
            return in;

        const int32_t width = in.width();
        const int32_t height = in.height();
        if ( blurRadius > width )
            blurRadius = width;
        if ( blurRadius > height )
            blurRadius = height;

        Image out( width, height );
        std::fill( out.transform(), out.transform() + width * height, static_cast<uint8_t>( 0 ) );

        uint8_t * imageOutY = out.image();
        const uint8_t * imageIn = in.image();

        const uint8_t * gamePalette = getGamePalette();

        for ( int32_t y = 0; y < height; ++y, imageOutY += width ) {
            uint8_t * imageOutX = imageOutY;

            int32_t startY = y - blurRadius;
            int32_t endY = y + blurRadius;
            if ( startY < 0 )
                startY = 0;
            if ( endY > height )
                endY = height;

            const int32_t roiHeight = endY - startY;

            const uint8_t * imageInStartY = imageIn + startY * width;

            for ( int32_t x = 0; x < width; ++x, ++imageOutX ) {
                int32_t startX = x - blurRadius;
                int32_t endX = x + blurRadius;
                if ( startX < 0 )
                    startX = 0;
                if ( endX > width )
                    endX = width;

                const int32_t roiWidth = endX - startX;

                uint32_t sumRed = 0;
                uint32_t sumGreen = 0;
                uint32_t sumBlue = 0;

                const uint8_t * imageInY = imageInStartY + startX;
                const uint8_t * imageInYEnd = imageInY + roiHeight * width;

                for ( ; imageInY != imageInYEnd; imageInY += width ) {
                    const uint8_t * imageInX = imageInY;
                    const uint8_t * imageInXEnd = imageInX + roiWidth;
                    for ( ; imageInX != imageInXEnd; ++imageInX ) {
                        const uint8_t * palette = gamePalette + *imageInX * 3;

                        sumRed += ( *palette );
                        ++palette;
                        sumGreen += ( *palette );
                        ++palette;
                        sumBlue += ( *palette );
                        ++palette;
                    }
                }

                const uint32_t roiSize = static_cast<uint32_t>( roiWidth * roiHeight );

                *imageOutX
                    = GetPALColorId( static_cast<uint8_t>( sumRed / roiSize ), static_cast<uint8_t>( sumGreen / roiSize ), static_cast<uint8_t>( sumBlue / roiSize ) );
            }
        }

        return out;
    }

    Sprite CreateContour( const Image & image, uint8_t value )
    {
        const int32_t width = image.width();
        const int32_t height = image.height();

        Sprite contour( width, height );
        contour.reset();
        if ( width < 2 || height < 2 ) {
            return contour;
        }

        assert( !contour.empty() );

        const uint8_t * inY = image.transform();
        uint8_t * outImageY = contour.image();
        uint8_t * outTransformY = contour.transform();

        const int32_t reducedWidth = width - 1;
        const int32_t reducedHeight = height - 1;

        for ( int32_t y = 0; y < height; ++y, inY += width, outImageY += width, outTransformY += width ) {
            const uint8_t * inX = inY;

            const bool isNotTopRow = ( y > 0 );
            const bool isNotBottomRow = ( y < reducedHeight );

            for ( int32_t x = 0; x < width; ++x, ++inX ) {
                if ( *inX > 0 && *inX < 6 ) { // 1 is to skip, 2 - 5 types of shadows
                    if ( ( x > 0 && *( inX - 1 ) == 0 ) || ( x < reducedWidth && *( inX + 1 ) == 0 ) || ( isNotTopRow && *( inX - width ) == 0 )
                         || ( isNotBottomRow && *( inX + width ) == 0 ) ) {
                        outImageY[x] = value;
                        outTransformY[x] = 0;
                    }
                }
            }
        }

        return contour;
    }

    Sprite Crop( const Image & image, int32_t x, int32_t y, int32_t width, int32_t height )
    {
        if ( image.empty() || width <= 0 || height <= 0 )
            return Sprite();

        if ( x < 0 ) {
            const int32_t offsetX = -x;
            if ( offsetX >= width )
                return Sprite();

            x = 0;
            width -= offsetX;
        }

        if ( y < 0 ) {
            const int32_t offsetY = -y;
            if ( offsetY >= height )
                return Sprite();

            y = 0;
            height -= offsetY;
        }

        if ( x > image.width() || y > image.height() )
            return Sprite();

        if ( x + width > image.width() ) {
            const int32_t offsetX = x + width - image.width();
            width -= offsetX;
        }

        if ( y + height > image.height() ) {
            const int32_t offsetY = y + height - image.height();
            height -= offsetY;
        }

        Sprite out( width, height );
        if ( image.singleLayer() ) {
            out._disableTransformLayer();
        }

        Copy( image, x, y, out, 0, 0, width, height );
        out.setPosition( x, y );
        return out;
    }

    void DrawBorder( Image & image, uint8_t value, uint32_t skipFactor )
    {
        if ( image.empty() || image.width() < 2 || image.height() < 2 ) {
            return;
        }

        const int32_t width = image.width();
        const int32_t height = image.height();

        if ( skipFactor < 2 ) {
            // top side
            uint8_t * data = image.image();
            uint8_t * transform = image.transform();
            const uint8_t * dataEnd = data + width;
            for ( ; data != dataEnd; ++data, ++transform ) {
                *data = value;
                *transform = 0;
            }

            // bottom side
            data = image.image() + width * ( height - 1 );
            transform = image.transform() + width * ( height - 1 );
            dataEnd = data + width;
            for ( ; data != dataEnd; ++data, ++transform ) {
                *data = value;
                *transform = 0;
            }

            // left side
            data = image.image() + width;
            transform = image.transform() + width;
            dataEnd = data + width * ( height - 2 );
            for ( ; data != dataEnd; data += width, transform += width ) {
                *data = value;
                *transform = 0;
            }

            // right side
            data = image.image() + width + width - 1;
            transform = image.transform() + width + width - 1;
            dataEnd = data + width * ( height - 2 );
            for ( ; data != dataEnd; data += width, transform += width ) {
                *data = value;
                *transform = 0;
            }
        }
        else {
            uint32_t counter = 0;

            // top side
            uint8_t * data = image.image();
            uint8_t * transform = image.transform();
            const uint8_t * dataEnd = data + width;
            for ( ; data != dataEnd; ++data, ++transform ) {
                if ( counter != skipFactor ) {
                    *data = value;
                    *transform = 0;
                }
                else {
                    counter = 0;
                }
                ++counter;
            }

            // right side
            data = image.image() + width + width - 1;
            transform = image.transform() + width + width - 1;
            dataEnd = data + width * ( height - 2 );
            for ( ; data != dataEnd; data += width, transform += width ) {
                if ( counter != skipFactor ) {
                    *data = value;
                    *transform = 0;
                }
                else {
                    counter = 0;
                }
                ++counter;
            }

            // bottom side
            data = image.image() + width * ( height - 1 ) + width - 1;
            transform = image.transform() + width * ( height - 1 ) + width - 1;
            dataEnd = data - width;
            for ( ; data != dataEnd; --data, --transform ) {
                if ( counter != skipFactor ) {
                    *data = value;
                    *transform = 0;
                }
                else {
                    counter = 0;
                }
                ++counter;
            }

            // left side
            data = image.image() + width * ( height - 2 );
            transform = image.transform() + width * ( height - 2 );
            dataEnd = image.image();
            for ( ; data != dataEnd; data -= width, transform -= width ) {
                if ( counter != skipFactor ) {
                    *data = value;
                    *transform = 0;
                }
                else {
                    counter = 0;
                }
                ++counter;
            }
        }
    }

    void DrawLine( Image & image, const Point & start, const Point & end, uint8_t value, const Rect & roi )
    {
        if ( image.empty() )
            return;

        const int32_t width = image.width();
        const int32_t height = image.height();

        int32_t x1 = start.x;
        int32_t y1 = start.y;
        int32_t x2 = end.x;
        int32_t y2 = end.y;

        const int32_t dx = std::abs( x2 - x1 );
        const int32_t dy = std::abs( y2 - y1 );

        const bool isValidRoi = roi.width > 0 && roi.height > 0;

        const int32_t minX = isValidRoi ? std::max( roi.x, 0 ) : 0;
        const int32_t minY = isValidRoi ? std::max( roi.y, 0 ) : 0;
        int32_t maxX = isValidRoi ? roi.x + roi.width : width;
        int32_t maxY = isValidRoi ? roi.y + roi.height : height;

        if ( minX >= width || minY >= height )
            return;

        if ( maxX >= width )
            maxX = width;

        if ( maxY >= height )
            maxY = height;

        uint8_t * data = image.image();
        uint8_t * transform = image.transform();

        if ( dx > dy ) {
            int32_t ns = dx / 2;

            for ( int32_t i = 0; i <= dx; ++i ) {
                if ( x1 >= minX && x1 < maxX && y1 >= minY && y1 < maxY ) {
                    const int32_t offset = x1 + y1 * width;
                    *( data + offset ) = value;
                    *( transform + offset ) = 0;
                }
                x1 < x2 ? ++x1 : --x1;
                ns -= dy;
                if ( ns < 0 ) {
                    y1 < y2 ? ++y1 : --y1;
                    ns += dx;
                }
            }
        }
        else {
            int32_t ns = dy / 2;

            for ( int32_t i = 0; i <= dy; ++i ) {
                if ( x1 >= minX && x1 < maxX && y1 >= minY && y1 < maxY ) {
                    const int32_t offset = x1 + y1 * width;
                    *( data + offset ) = value;
                    *( transform + offset ) = 0;
                }
                y1 < y2 ? ++y1 : --y1;
                ns -= dx;
                if ( ns < 0 ) {
                    x1 < x2 ? ++x1 : --x1;
                    ns += dy;
                }
            }
        }
    }

    void DrawRect( Image & image, const Rect & roi, uint8_t value )
    {
        if ( image.empty() || roi.width < 1 || roi.height < 1 )
            return;

        DrawLine( image, { roi.x, roi.y }, { roi.x + roi.width, roi.y }, value, roi );
        DrawLine( image, { roi.x, roi.y }, { roi.x, roi.y + roi.height }, value, roi );
        DrawLine( image, { roi.x + roi.width - 1, roi.y }, { roi.x + roi.width - 1, roi.y + roi.height }, value, roi );
        DrawLine( image, { roi.x, roi.y + roi.height - 1 }, { roi.x + roi.width, roi.y + roi.height - 1 }, value, roi );
    }

    void DivideImageBySquares( const Point & spriteOffset, const Image & original, const int32_t squareSize, const bool flip,
                               std::vector<std::pair<Point, Sprite>> & output )
    {
        if ( original.empty() ) {
            return;
        }

        if ( squareSize <= 0 ) {
            assert( 0 );
            return;
        }

        Point offset{ spriteOffset.x / squareSize, spriteOffset.y / squareSize };

        // The start of a square must be before image offset so in case of negative offset we need to decrease the ID of the start square.
        if ( ( spriteOffset.x < 0 ) && ( offset.x * squareSize != spriteOffset.x ) ) {
            --offset.x;
        }

        if ( ( spriteOffset.y < 0 ) && ( offset.y * squareSize != spriteOffset.y ) ) {
            --offset.y;
        }

        const Point spriteRelativeOffset{ spriteOffset.x - offset.x * squareSize, spriteOffset.y - offset.y * squareSize };
        const Point stepPerDirection{ ( original.width() + spriteRelativeOffset.x + squareSize - 1 ) / squareSize,
                                      ( original.height() + spriteRelativeOffset.y + squareSize - 1 ) / squareSize };
        assert( stepPerDirection.x > 0 && stepPerDirection.y > 0 );

        const Rect relativeROI( spriteRelativeOffset.x, spriteRelativeOffset.y, original.width(), original.height() );

        for ( int32_t y = 0; y < stepPerDirection.y; ++y ) {
            for ( int32_t x = 0; x < stepPerDirection.x; ++x ) {
                const Rect roi( x * squareSize, y * squareSize, squareSize, squareSize );
                const Rect intersection = relativeROI ^ roi;
                assert( intersection.width > 0 && intersection.height > 0 );

                if ( flip ) {
                    Sprite cropped( intersection.width, intersection.height );
                    Flip( original, original.width() - intersection.x + spriteRelativeOffset.x - intersection.width, intersection.y - spriteRelativeOffset.y, cropped, 0,
                          0, intersection.width, intersection.height, true, false );

                    assert( !cropped.empty() );
                    cropped.setPosition( intersection.x - roi.x, intersection.y - roi.y );

                    output.emplace_back( offset + Point( x, y ), std::move( cropped ) );
                }
                else {
                    Sprite cropped
                        = Crop( original, intersection.x - spriteRelativeOffset.x, intersection.y - spriteRelativeOffset.y, intersection.width, intersection.height );
                    assert( !cropped.empty() );
                    cropped.setPosition( intersection.x - roi.x, intersection.y - roi.y );

                    output.emplace_back( offset + Point( x, y ), std::move( cropped ) );
                }
            }
        }
    }

    Image ExtractCommonPattern( const std::vector<const Image *> & input )
    {
        if ( input.empty() )
            return Image();

        assert( input.front() != nullptr );

        if ( input.size() == 1 ) {
            return *input.front();
        }

        if ( input[0]->empty() )
            return Image();

        for ( size_t i = 1; i < input.size(); ++i ) {
            assert( input[i] != nullptr );
            if ( input[i]->width() != input[0]->width() || input[i]->height() != input[0]->height() )
                return Image();
        }

        std::vector<const uint8_t *> imageIn( input.size() );
        std::vector<const uint8_t *> transformIn( input.size() );

        for ( size_t i = 0; i < input.size(); ++i ) {
            imageIn[i] = input[i]->image();
            transformIn[i] = input[i]->transform();
        }

        Image out( input[0]->width(), input[0]->height() );
        out.reset();

        uint8_t * imageOut = out.image();
        uint8_t * transformOut = out.transform();
        const uint8_t * imageOutEnd = imageOut + out.width() * out.height();

        bool isEqual = false;

        for ( ; imageOut != imageOutEnd; ++imageOut, ++transformOut ) {
            isEqual = true;

            for ( size_t i = 1; i < input.size(); ++i ) {
                if ( *imageIn[0] != *imageIn[i] || *transformIn[0] != *transformIn[i] ) {
                    isEqual = false;
                    break;
                }
            }

            if ( isEqual ) {
                *imageOut = *imageIn[0];
                *transformOut = *transformIn[0];
            }

            for ( size_t i = 0; i < input.size(); ++i ) {
                ++imageIn[i];
                ++transformIn[i];
            }
        }

        return out;
    }

    void Fill( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t colorId )
    {
        if ( !Verify( image, x, y, width, height ) )
            return;

        if ( image.width() == width && image.height() == height ) { // we need to fill whole image
            image.fill( colorId );
            return;
        }

        const int32_t imageWidth = image.width();

        uint8_t * imageY = image.image() + y * imageWidth + x;
        uint8_t * transformY = image.transform() + y * imageWidth + x;
        const uint8_t * imageYEnd = imageY + height * imageWidth;

        for ( ; imageY != imageYEnd; imageY += imageWidth, transformY += imageWidth ) {
            std::fill( imageY, imageY + width, colorId );
            std::fill( transformY, transformY + width, static_cast<uint8_t>( 0 ) );
        }
    }

    void FillTransform( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t tranformId )
    {
        if ( !Verify( image, x, y, width, height ) )
            return;

        const int32_t imageWidth = image.width();

        uint8_t * imageY = image.image() + y * imageWidth + x;
        uint8_t * transformY = image.transform() + y * imageWidth + x;
        const uint8_t * imageYEnd = imageY + height * imageWidth;

        for ( ; imageY != imageYEnd; imageY += imageWidth, transformY += imageWidth ) {
            std::fill( imageY, imageY + width, static_cast<uint8_t>( 0 ) );
            std::fill( transformY, transformY + width, tranformId );
        }
    }

    Image FilterOnePixelNoise( const Image & input )
    {
        if ( input.width() < 3 || input.height() < 3 ) {
            return input;
        }

        const int32_t width = input.width();
        const int32_t height = input.height();

        Image output( width, height );
        output.reset();

        const uint8_t * imageInY = input.image();
        const uint8_t * transformInY = input.transform();

        uint8_t * imageOutY = output.image();
        uint8_t * transformOutY = output.transform();

        for ( int32_t y = 0; y < height; ++y ) {
            const uint8_t * transformInX = transformInY;

            for ( int32_t x = 0; x < width; ++x ) {
                if ( *transformInX == 0 && ( x == 0 || x == width - 1 || *( transformInX - 1 ) == 0 || *( transformInX + 1 ) == 0 )
                     && ( y == 0 || y == height - 1 || *( transformInX - width ) == 0 || *( transformInX + width ) == 0 ) ) {
                    *( transformOutY + x ) = 0;
                    *( imageOutY + x ) = *( imageInY + x );
                }

                ++transformInX;
            }

            imageInY += width;
            transformInY += width;

            imageOutY += width;
            transformOutY += width;
        }

        return output;
    }

    bool FitToRoi( const Image & in, Point & inPos, const Image & out, Point & outPos, Size & outputSize, const Rect & outputRoi )
    {
        if ( !Validate( out, outputRoi.x, outputRoi.y, outputRoi.width, outputRoi.height ) ) {
            return false;
        }

        outPos.x -= outputRoi.x;
        outPos.y -= outputRoi.y;

        if ( !Verify( inPos.x, inPos.y, outPos.x, outPos.y, outputSize.width, outputSize.height, in.width(), in.height(), outputRoi.width, outputRoi.height ) ) {
            return false;
        }

        outPos.x += outputRoi.x;
        outPos.y += outputRoi.y;

        return true;
    }

    Image Flip( const Image & in, bool horizontally, bool vertically )
    {
        if ( in.empty() ) {
            return Image();
        }

        const int32_t width = in.width();
        const int32_t height = in.height();

        Image out( width, height );
        if ( !horizontally && !vertically ) {
            Copy( in, out );
            return out;
        }

        Flip( in, 0, 0, out, 0, 0, width, height, horizontally, vertically );
        return out;
    }

    void Flip( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, bool horizontally, bool vertically )
    {
        if ( !Verify( in, inX, inY, out, outX, outY, width, height ) ) {
            return;
        }

        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();

        const int32_t offsetOut = outY * widthOut + outX;
        const int32_t offsetIn = inY * widthIn + inX;

        uint8_t * imageOutY = out.image() + offsetOut;
        uint8_t * transformOutY = out.transform() + offsetOut;
        const uint8_t * imageOutYEnd = imageOutY + widthOut * height;

        if ( horizontally && !vertically ) {
            const uint8_t * imageInY = in.image() + offsetIn + width - 1;
            const uint8_t * transformInY = in.transform() + offsetIn + width - 1;

            for ( ; imageOutY != imageOutYEnd; imageOutY += widthOut, transformOutY += widthOut, imageInY += widthIn, transformInY += widthIn ) {
                uint8_t * imageOutX = imageOutY;
                uint8_t * transformOutX = transformOutY;
                const uint8_t * imageOutXEnd = imageOutX + width;
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformInX = transformInY;

                for ( ; imageOutX != imageOutXEnd; ++imageOutX, ++transformOutX, --imageInX, --transformInX ) {
                    *imageOutX = *imageInX;
                    *transformOutX = *transformInX;
                }
            }
        }
        else if ( !horizontally && vertically ) {
            const uint8_t * imageInY = in.image() + offsetIn + ( height - 1 ) * widthIn;
            const uint8_t * transformInY = in.transform() + offsetIn + ( height - 1 ) * widthIn;

            for ( ; imageOutY != imageOutYEnd; imageOutY += widthOut, transformOutY += widthOut, imageInY -= widthIn, transformInY -= widthIn ) {
                memcpy( imageOutY, imageInY, static_cast<size_t>( width ) );
                memcpy( transformOutY, transformInY, static_cast<size_t>( width ) );
            }
        }
        else {
            const uint8_t * imageInY = in.image() + offsetIn + ( height - 1 ) * widthIn + widthIn - 1;
            const uint8_t * transformInY = in.transform() + offsetIn + ( height - 1 ) * widthIn + widthIn - 1;

            for ( ; imageOutY != imageOutYEnd; imageOutY += widthOut, transformOutY += widthOut, imageInY -= widthIn, transformInY -= widthIn ) {
                uint8_t * imageOutX = imageOutY;
                uint8_t * transformOutX = transformOutY;
                const uint8_t * imageOutXEnd = imageOutX + width;
                const uint8_t * imageInX = imageInY;
                const uint8_t * transformInX = transformInY;

                for ( ; imageOutX != imageOutXEnd; ++imageOutX, ++transformOutX, --imageInX, --transformInX ) {
                    *imageOutX = *imageInX;
                    *transformOutX = *transformInX;
                }
            }
        }
    }

    Rect GetActiveROI( const Image & image, const uint8_t minTransformValue )
    {
        if ( image.empty() )
            return Rect();

        const int32_t width = image.width();
        const int32_t height = image.height();

        Rect area( -1, -1, -1, -1 );

        // Top border
        const uint8_t * inY = image.transform();
        for ( int32_t y = 0; y < height; ++y, inY += width ) {
            const uint8_t * inX = inY;

            for ( int32_t x = 0; x < width; ++x, ++inX ) {
                if ( *inX == 0 || *inX >= minTransformValue ) {
                    area.y = y;
                    break;
                }
            }

            if ( area.y >= 0 )
                break;
        }

        if ( area.y < 0 )
            return Rect();

        // Bottom border
        inY = image.transform() + width * ( height - 1 );
        for ( int32_t y = height - 1; y > area.y; --y, inY -= width ) {
            const uint8_t * inX = inY;

            for ( int32_t x = 0; x < width; ++x, ++inX ) {
                if ( *inX == 0 || *inX >= minTransformValue ) {
                    area.height = y - area.y + 1;
                    break;
                }
            }

            if ( area.height >= 0 )
                break;
        }

        // Left border
        const uint8_t * inX = image.transform() + width * area.y;
        for ( int32_t x = 0; x < width; ++x, ++inX ) {
            inY = inX;

            for ( int32_t y = 0; y < area.height; ++y, inY += width ) {
                if ( *inY == 0 || *inY >= minTransformValue ) {
                    area.x = x;
                    break;
                }
            }

            if ( area.x >= 0 )
                break;
        }

        // Right border
        inX = image.transform() + width * area.y + width - 1;
        for ( int32_t x = width - 1; x >= area.x; --x, --inX ) {
            inY = inX;

            for ( int32_t y = 0; y < area.height; ++y, inY += width ) {
                if ( *inY == 0 || *inY >= minTransformValue ) {
                    area.width = x - area.x + 1;
                    break;
                }
            }

            if ( area.width >= 0 )
                break;
        }

        return area;
    }

    uint8_t GetColorId( uint8_t red, uint8_t green, uint8_t blue )
    {
        return GetPALColorId( red / 4, green / 4, blue / 4 );
    }

    std::vector<uint8_t> getTransformTable( const Image & in, const Image & out, int32_t x, int32_t y, int32_t width, int32_t height )
    {
        std::vector<uint8_t> table( 256 );
        for ( size_t i = 0; i < table.size(); ++i ) {
            table[i] = static_cast<uint8_t>( i );
        }

        if ( !Verify( in, x, y, width, height ) ) {
            return table;
        }

        if ( in.width() != out.width() || in.height() != out.height() ) {
            return table;
        }

        const int32_t imageWidth = in.width();

        const int32_t offset = y * imageWidth + x;

        const uint8_t * imageInY = in.image() + offset;
        const uint8_t * imageInYEnd = imageInY + height * imageWidth;
        const uint8_t * imageOutY = out.image() + offset;
        const uint8_t * transformInY = in.transform() + offset;
        const uint8_t * transformOutY = out.transform() + offset;

        for ( ; imageInY != imageInYEnd; imageInY += imageWidth, transformInY += imageWidth, imageOutY += imageWidth, transformOutY += imageWidth ) {
            const uint8_t * imageInX = imageInY;
            const uint8_t * transformInX = transformInY;
            const uint8_t * imageOutX = imageOutY;
            const uint8_t * transformOutX = transformOutY;
            const uint8_t * imageInXEnd = imageInX + width;

            for ( ; imageInX != imageInXEnd; ++imageInX, ++transformInX, ++imageOutX, ++transformOutX ) {
                if ( *transformInX != *transformOutX ) {
                    continue;
                }

                if ( *transformInX != 0 ) {
                    continue;
                }

                table[*imageInX] = *imageOutX;
            }
        }

        return table;
    }

    Sprite makeShadow( const Sprite & in, const Point & shadowOffset, const uint8_t transformId )
    {
        if ( in.empty() || shadowOffset.x > 0 || shadowOffset.y < 0 )
            return Sprite();

        const int32_t width = in.width();
        const int32_t height = in.height();

        // Shadow has (-x, +y) offset.
        Sprite out( width - shadowOffset.x, height + shadowOffset.y, in.x() + shadowOffset.x, in.y() );
        out.reset();

        assert( !out.empty() );

        const int32_t widthOut = out.width();

        const uint8_t * transformInY = in.transform();
        const uint8_t * transformInYEnd = transformInY + width * height;
        uint8_t * transformOutY = out.transform() + shadowOffset.y * widthOut;

        for ( ; transformInY != transformInYEnd; transformInY += width, transformOutY += widthOut ) {
            const uint8_t * transformInX = transformInY;
            uint8_t * transformOutX = transformOutY;
            const uint8_t * transformInXEnd = transformInX + width;

            for ( ; transformInX != transformInXEnd; ++transformInX, ++transformOutX ) {
                if ( *transformInX == 0 ) {
                    *transformOutX = transformId;
                }
            }
        }

        return out;
    }

    void MaskTransformLayer( const Image & mask, int32_t maskX, int32_t maskY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height )
    {
        if ( !Verify( mask, maskX, maskY, out, outX, outY, width, height ) ) {
            return;
        }

        const int32_t widthMask = mask.width();
        const int32_t widthOut = out.width();

        const uint8_t * imageMaskY = mask.transform() + maskY * widthMask + maskX;
        uint8_t * imageOutY = out.transform() + outY * widthOut + outX;
        const uint8_t * imageMaskYEnd = imageMaskY + height * widthMask;

        for ( ; imageMaskY != imageMaskYEnd; imageMaskY += widthMask, imageOutY += widthOut ) {
            const uint8_t * imageMaskX = imageMaskY;
            uint8_t * imageOutX = imageOutY;
            const uint8_t * imageMaskXEnd = imageMaskX + width;

            for ( ; imageMaskX != imageMaskXEnd; ++imageMaskX, ++imageOutX ) {
                if ( *imageMaskX == 0 ) {
                    *imageOutX = 1;
                }
            }
        }
    }

    void ReplaceColorId( Image & image, uint8_t oldColorId, uint8_t newColorId )
    {
        if ( image.empty() )
            return;

        uint8_t * data = image.image();
        const uint8_t * dataEnd = data + image.width() * image.height();

        for ( ; data != dataEnd; ++data ) {
            if ( *data == oldColorId ) {
                *data = newColorId;
            }
        }
    }

    void ReplaceColorIdByTransformId( Image & image, uint8_t colorId, uint8_t transformId )
    {
        if ( transformId > 15 )
            return;

        const int32_t width = image.width();
        const int32_t height = image.height();

        const uint8_t * imageIn = image.image();
        uint8_t * transformIn = image.transform();
        const uint8_t * imageInEnd = imageIn + height * width;
        for ( ; imageIn != imageInEnd; ++imageIn, ++transformIn ) {
            if ( *transformIn == 0 && *imageIn == colorId ) { // modify pixels with tranform value 0
                *transformIn = transformId;
            }
        }
    }

    void Resize( const Image & in, Image & out, const bool isSubpixelAccuracy )
    {
        if ( in.empty() || out.empty() ) {
            return;
        }

        Resize( in, 0, 0, in.width(), in.height(), out, 0, 0, out.width(), out.height(), isSubpixelAccuracy );
    }

    void Resize( const Image & in, const int32_t inX, const int32_t inY, const int32_t widthRoiIn, const int32_t heightRoiIn, Image & out, const int32_t outX,
                 const int32_t outY, const int32_t widthRoiOut, const int32_t heightRoiOut, const bool isSubpixelAccuracy )
    {
        if ( !Validate( in, inX, inY, widthRoiIn, heightRoiIn ) || !Validate( out, outX, outY, widthRoiOut, heightRoiOut ) )
            return;

        if ( widthRoiIn == widthRoiOut && heightRoiIn == heightRoiOut ) {
            Copy( in, inX, inY, out, outX, outY, widthRoiIn, heightRoiIn );
            return;
        }

        const int32_t widthIn = in.width();
        const int32_t widthOut = out.width();

        const int32_t offsetInY = inY * widthIn + inX;
        const int32_t offsetOutY = outY * widthOut + outX;

        const uint8_t * imageInY = in.image() + offsetInY;
        uint8_t * imageOutY = out.image() + offsetOutY;

        if ( isSubpixelAccuracy ) {
            std::vector<double> positionX( widthRoiOut );
            std::vector<double> positionY( heightRoiOut );
            for ( int32_t x = 0; x < widthRoiOut; ++x )
                positionX[x] = static_cast<double>( x * widthRoiIn ) / widthRoiOut;
            for ( int32_t y = 0; y < heightRoiOut; ++y )
                positionY[y] = static_cast<double>( y * heightRoiIn ) / heightRoiOut;

            const uint8_t * gamePalette = getGamePalette();

            if ( in.singleLayer() && out.singleLayer() ) {
                for ( int32_t y = 0; y < heightRoiOut; ++y, imageOutY += widthOut ) {
                    const double posY = positionY[y];
                    const int32_t startY = static_cast<int32_t>( posY );
                    const double coeffY = posY - startY;

                    uint8_t * imageOutX = imageOutY;

                    for ( int32_t x = 0; x < widthRoiOut; ++x, ++imageOutX ) {
                        const double posX = positionX[x];
                        const int32_t startX = static_cast<int32_t>( posX );
                        const int32_t offsetIn = startY * widthIn + startX;

                        const uint8_t * imageInX = imageInY + offsetIn;

                        if ( posX < widthRoiIn - 1 && posY < heightRoiIn - 1 ) {
                            const double coeffX = posX - startX;
                            const double coeff1 = ( 1 - coeffX ) * ( 1 - coeffY );
                            const double coeff2 = coeffX * ( 1 - coeffY );
                            const double coeff3 = ( 1 - coeffX ) * coeffY;
                            const double coeff4 = coeffX * coeffY;

                            const uint8_t * id1 = gamePalette + static_cast<uint32_t>( *imageInX ) * 3;
                            const uint8_t * id2 = gamePalette + static_cast<uint32_t>( *( imageInX + 1 ) ) * 3;
                            const uint8_t * id3 = gamePalette + static_cast<uint32_t>( *( imageInX + widthIn ) ) * 3;
                            const uint8_t * id4 = gamePalette + static_cast<uint32_t>( *( imageInX + widthIn + 1 ) ) * 3;

                            const double red = *id1 * coeff1 + *id2 * coeff2 + *id3 * coeff3 + *id4 * coeff4 + 0.5;
                            const double green = *( id1 + 1 ) * coeff1 + *( id2 + 1 ) * coeff2 + *( id3 + 1 ) * coeff3 + *( id4 + 1 ) * coeff4 + 0.5;
                            const double blue = *( id1 + 2 ) * coeff1 + *( id2 + 2 ) * coeff2 + *( id3 + 2 ) * coeff3 + *( id4 + 2 ) * coeff4 + 0.5;

                            *imageOutX = GetPALColorId( static_cast<uint8_t>( red ), static_cast<uint8_t>( green ), static_cast<uint8_t>( blue ) );
                        }
                        else {
                            *imageOutX = *imageInX;
                        }
                    }
                }
            }
            else {
                const uint8_t * transformInY = in.transform() + offsetInY;
                uint8_t * transformOutY = out.transform() + offsetOutY;

                for ( int32_t y = 0; y < heightRoiOut; ++y, imageOutY += widthOut, transformOutY += widthOut ) {
                    const double posY = positionY[y];
                    const int32_t startY = static_cast<int32_t>( posY );
                    const double coeffY = posY - startY;

                    uint8_t * imageOutX = imageOutY;
                    uint8_t * transformOutX = transformOutY;

                    for ( int32_t x = 0; x < widthRoiOut; ++x, ++imageOutX, ++transformOutX ) {
                        const double posX = positionX[x];
                        const int32_t startX = static_cast<int32_t>( posX );
                        const int32_t offsetIn = startY * widthIn + startX;

                        const uint8_t * imageInX = imageInY + offsetIn;
                        const uint8_t * transformInX = transformInY + offsetIn;

                        if ( posX < widthIn - 1 && posY < heightRoiIn - 1 ) {
                            if ( *transformInX == 0 && *( transformInX + 1 ) == 0 && *( transformInX + widthRoiIn ) == 0 && *( transformInX + widthRoiIn + 1 ) == 0 ) {
                                const double coeffX = posX - startX;
                                const double coeff1 = ( 1 - coeffX ) * ( 1 - coeffY );
                                const double coeff2 = coeffX * ( 1 - coeffY );
                                const double coeff3 = ( 1 - coeffX ) * coeffY;
                                const double coeff4 = coeffX * coeffY;

                                const uint8_t * id1 = gamePalette + static_cast<uint32_t>( *imageInX ) * 3;
                                const uint8_t * id2 = gamePalette + static_cast<uint32_t>( *( imageInX + 1 ) ) * 3;
                                const uint8_t * id3 = gamePalette + static_cast<uint32_t>( *( imageInX + widthIn ) ) * 3;
                                const uint8_t * id4 = gamePalette + static_cast<uint32_t>( *( imageInX + widthIn + 1 ) ) * 3;

                                const double red = *id1 * coeff1 + *id2 * coeff2 + *id3 * coeff3 + *id4 * coeff4 + 0.5;
                                const double green = *( id1 + 1 ) * coeff1 + *( id2 + 1 ) * coeff2 + *( id3 + 1 ) * coeff3 + *( id4 + 1 ) * coeff4 + 0.5;
                                const double blue = *( id1 + 2 ) * coeff1 + *( id2 + 2 ) * coeff2 + *( id3 + 2 ) * coeff3 + *( id4 + 2 ) * coeff4 + 0.5;

                                *imageOutX = GetPALColorId( static_cast<uint8_t>( red ), static_cast<uint8_t>( green ), static_cast<uint8_t>( blue ) );
                            }
                            else {
                                *imageOutX = *imageInX;
                            }
                        }
                        else {
                            *imageOutX = *imageInX;
                        }

                        *transformOutX = *transformInX;
                    }
                }
            }
        }
        else {
            const uint8_t * imageOutYEnd = imageOutY + widthOut * heightRoiOut;
            int32_t idY = 0;

            // Precalculation of X position
            std::vector<int32_t> positionX( widthRoiOut );
            for ( int32_t x = 0; x < widthRoiOut; ++x )
                positionX[x] = ( x * widthRoiIn ) / widthRoiOut;

            if ( in.singleLayer() && out.singleLayer() ) {
                for ( ; imageOutY != imageOutYEnd; imageOutY += widthOut, ++idY ) {
                    uint8_t * imageOutX = imageOutY;
                    const uint8_t * imageOutXEnd = imageOutX + widthRoiOut;

                    const int32_t offset = ( ( idY * heightRoiIn ) / heightRoiOut ) * widthIn;
                    const uint8_t * imageInX = imageInY + offset;
                    const int32_t * idX = positionX.data();

                    for ( ; imageOutX != imageOutXEnd; ++imageOutX, ++idX ) {
                        *imageOutX = *( imageInX + ( *idX ) );
                    }
                }
            }
            else {
                const uint8_t * transformInY = in.transform() + offsetInY;
                uint8_t * transformOutY = out.transform() + offsetOutY;

                for ( ; imageOutY != imageOutYEnd; imageOutY += widthOut, transformOutY += widthOut, ++idY ) {
                    uint8_t * imageOutX = imageOutY;
                    uint8_t * transformOutX = transformOutY;
                    const uint8_t * imageOutXEnd = imageOutX + widthRoiOut;

                    const int32_t offset = ( ( idY * heightRoiIn ) / heightRoiOut ) * widthIn;
                    const uint8_t * imageInX = imageInY + offset;
                    const uint8_t * transformInX = transformInY + offset;
                    const int32_t * idX = positionX.data();

                    for ( ; imageOutX != imageOutXEnd; ++imageOutX, ++transformOutX, ++idX ) {
                        *imageOutX = *( imageInX + ( *idX ) );
                        *transformOutX = *( transformInX + ( *idX ) );
                    }
                }
            }
        }
    }

    void SetPixel( Image & image, int32_t x, int32_t y, uint8_t value )
    {
        if ( image.empty() || x >= image.width() || y >= image.height() || x < 0 || y < 0 ) {
            return;
        }

        const int32_t offset = y * image.width() + x;
        *( image.image() + offset ) = value;
        *( image.transform() + offset ) = 0;
    }

    void SetPixel( Image & image, const std::vector<Point> & points, uint8_t value )
    {
        if ( image.empty() ) {
            return;
        }

        const int32_t width = image.width();
        const int32_t height = image.height();

        for ( const Point & point : points ) {
            if ( point.x >= width || point.y >= height || point.x < 0 || point.y < 0 ) {
                continue;
            }

            const int32_t offset = point.y * width + point.x;
            *( image.image() + offset ) = value;
            *( image.transform() + offset ) = 0;
        }
    }

    void SetTransformPixel( Image & image, int32_t x, int32_t y, uint8_t value )
    {
        if ( image.empty() || x >= image.width() || y >= image.height() || x < 0 || y < 0 ) {
            return;
        }

        const int32_t offset = y * image.width() + x;
        *( image.image() + offset ) = 0;
        *( image.transform() + offset ) = value;
    }

    Image Stretch( const Image & in, int32_t inX, int32_t inY, int32_t widthIn, int32_t heightIn, int32_t widthOut, int32_t heightOut )
    {
        if ( !Validate( in, inX, inY, widthIn, heightIn ) || widthOut <= 0 || heightOut <= 0 ) {
            return Image();
        }

        Image out( widthOut, heightOut );

        const int32_t minWidth = widthIn < widthOut ? widthIn : widthOut;
        const int32_t minHeight = heightIn < heightOut ? heightIn : heightOut;

        const int32_t cornerWidth = minWidth / 3;
        const int32_t cornerHeight = minHeight / 3;
        const int32_t cornerX = inX + ( widthIn - cornerWidth ) / 2;
        const int32_t cornerY = inY + ( heightIn - cornerHeight ) / 2;
        const int32_t bodyWidth = minWidth - 2 * cornerWidth;
        const int32_t bodyHeight = minHeight - 2 * cornerHeight;

        const int32_t outX = ( widthOut - ( widthOut / bodyWidth ) * bodyWidth ) / 2;
        const int32_t outY = ( heightOut - ( heightOut / bodyHeight ) * bodyHeight ) / 2;

        // Create internal area
        if ( bodyWidth < widthOut && bodyHeight < heightOut ) {
            for ( int32_t y = 0; y < ( heightOut / bodyHeight ); ++y ) {
                for ( int32_t x = 0; x < ( widthOut / bodyWidth ); ++x ) {
                    Copy( in, cornerX, cornerY, out, outX + x * bodyWidth, outY + y * bodyHeight, bodyWidth, bodyHeight );
                }
            }
        }

        // Create sides
        // top and bottom side
        for ( int32_t x = 0; x < ( widthOut / bodyWidth ); ++x ) {
            const int32_t offsetX = outX + x * bodyWidth;
            Copy( in, cornerX, inY, out, offsetX, 0, bodyWidth, cornerHeight );
            Copy( in, cornerX, inY + heightIn - cornerHeight, out, offsetX, heightOut - cornerHeight, bodyWidth, cornerHeight );
        }

        // left and right sides
        for ( int32_t y = 0; y < ( heightOut / bodyHeight ); ++y ) {
            const int32_t offsetY = outY + y * bodyHeight;
            Copy( in, inX, cornerY, out, 0, offsetY, cornerWidth, bodyHeight );
            Copy( in, inX + widthIn - cornerWidth, cornerY, out, widthOut - cornerWidth, offsetY, cornerWidth, bodyHeight );
        }

        // Create corners
        // top-left corner
        Copy( in, inX, inY, out, 0, 0, cornerWidth, cornerHeight );

        // top-right corner
        Copy( in, inX + widthIn - cornerWidth, inY, out, widthOut - cornerWidth, 0, cornerWidth, cornerHeight );

        // bottom-left corner
        Copy( in, inX, inY + heightIn - cornerHeight, out, 0, heightOut - cornerHeight, cornerWidth, cornerHeight );

        // bottom-right corner
        Copy( in, inX + widthIn - cornerWidth, inY + heightIn - cornerHeight, out, widthOut - cornerWidth, heightOut - cornerHeight, cornerWidth, cornerHeight );

        return out;
    }

    void Transpose( const Image & in, Image & out )
    {
        assert( !out.empty() );

        if ( in.empty() || in.width() != out.height() || in.height() != out.width() ) {
            out.reset();
            return;
        }
        const int32_t width = in.width();
        const int32_t height = in.height();

        const uint8_t * imageInY = in.image();
        const uint8_t * imageInYEnd = imageInY + width * height;
        uint8_t * imageOutX = out.image();

        const uint8_t * transformInY = in.transform();
        uint8_t * transformOutX = out.transform();

        for ( ; imageInY != imageInYEnd; imageInY += width, transformInY += width, ++imageOutX, ++transformOutX ) {
            const uint8_t * imageInX = imageInY;
            const uint8_t * imageInXEnd = imageInX + width;
            uint8_t * imageOutY = imageOutX;

            const uint8_t * transformInX = transformInY;
            uint8_t * transformOutY = transformOutX;
            for ( ; imageInX != imageInXEnd; ++imageInX, ++transformInX, imageOutY += height, transformOutY += height ) {
                *imageOutY = *imageInX;
                *transformOutY = *transformInX;
            }
        }
    }

    void updateShadow( Image & image, const Point & shadowOffset, const uint8_t transformId )
    {
        if ( image.empty() || ( std::abs( shadowOffset.x ) >= image.width() ) || ( std::abs( shadowOffset.y ) >= image.height() ) || shadowOffset == Point() )
            return;

        const int32_t width = image.width() - std::abs( shadowOffset.x );
        const int32_t height = image.height() - std::abs( shadowOffset.y );

        const int32_t imageWidth = image.width();

        const uint8_t * transformInY = image.transform();
        uint8_t * transformOutY = image.transform();

        if ( shadowOffset.x > 0 ) {
            transformOutY += shadowOffset.x;
        }
        else {
            transformInY -= shadowOffset.x;
        }

        if ( shadowOffset.y > 0 ) {
            transformOutY += imageWidth * shadowOffset.y;
        }
        else {
            transformInY -= imageWidth * shadowOffset.y;
        }

        const uint8_t * transformOutYEnd = transformOutY + imageWidth * height;

        for ( ; transformOutY != transformOutYEnd; transformInY += imageWidth, transformOutY += imageWidth ) {
            const uint8_t * transformInX = transformInY;
            uint8_t * transformOutX = transformOutY;
            const uint8_t * transformOutXEnd = transformOutX + width;

            for ( ; transformOutX != transformOutXEnd; ++transformInX, ++transformOutX ) {
                if ( *transformInX == 0 && *transformOutX == 1 ) {
                    *transformOutX = transformId;
                }
            }
        }
    }
}
