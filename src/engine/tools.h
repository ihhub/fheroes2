/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#pragma once

#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
std::string GetHexString( const T value, const int width = 8 )
{
    std::ostringstream stream;

    stream << "0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << value;

    return stream.str();
}

int Sign( const int i );

std::string StringTrim( std::string str );

std::string StringLower( std::string str );
std::string StringUpper( std::string str );

std::vector<std::string> StringSplit( const std::string_view str, const char sep );

void StringReplace( std::string & dst, const char * pred, const std::string_view src );

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
void StringReplace( std::string & dst, const char * pred, const T value )
{
    StringReplace( dst, pred, std::to_string( value ) );
}

// Returns the number of bits that are set in the number passed as an argument
constexpr int CountBits( uint32_t val )
{
    int res = 0;

    // Using Brian Kernighan's algorithm: https://yuminlee2.medium.com/brian-kernighans-algorithm-count-set-bits-in-a-number-18ab05edca93
    while ( val ) {
        val &= ( val - 1 );
        ++res;
    }

    return res;
}

namespace fheroes2
{
    uint32_t calculateCRC32( const uint8_t * data, const size_t length );

    template <size_t N>
    std::bitset<N> makeBitsetFromVector( const std::vector<int> & vector )
    {
        std::bitset<N> result;
        for ( const int index : vector ) {
            result.set( index, true );
        }
        return result;
    }

    void replaceStringEnding( std::string & output, const char * originalEnding, const char * correctedEnding );

    std::string abbreviateNumber( const int num );

    // Appends the given modifier to the end of the given string (e.g. "Coliseum +2")
    void appendModifierToString( std::string & str, const int mod );

    // Performs case-insensitive string comparison, suitable for string sorting purposes. Returns true if the first parameter is
    // "less than" the second, otherwise returns false.
    template <typename CharType>
    bool compareStringsCaseInsensitively( const std::basic_string<CharType> & lhs, const std::basic_string<CharType> & rhs )
    {
        typename std::basic_string<CharType>::const_iterator li = lhs.begin();
        typename std::basic_string<CharType>::const_iterator ri = rhs.begin();

        const std::locale currentGlobalLocale;

        while ( li != lhs.end() && ri != rhs.end() ) {
            const CharType lc = std::tolower( *li, currentGlobalLocale );
            const CharType rc = std::tolower( *ri, currentGlobalLocale );

            if ( lc < rc ) {
                return true;
            }
            if ( lc > rc ) {
                return false;
            }
            // The characters are "equal", so proceed to checking the next pair
            ++li;
            ++ri;
        }

        // We have reached the end of one (or both) of the strings, the first parameter is considered "less than" the second if it is shorter
        return li == lhs.end() && ri != rhs.end();
    }

    // Performs a checked conversion of an integer value of type From to an integer type To. Returns an empty std::optional<To> if
    // the source value does not fit into the target type.
    template <typename To, typename From, std::enable_if_t<std::is_integral_v<To> && std::is_integral_v<From>, bool> = true>
    constexpr std::optional<To> checkedCast( const From from )
    {
        // Both types have the same signedness
        if constexpr ( std::is_signed_v<From> == std::is_signed_v<To> ) {
            if ( from < std::numeric_limits<To>::min() || from > std::numeric_limits<To>::max() ) {
                return {};
            }
        }
        // From is signed, To is unsigned
        else if constexpr ( std::is_signed_v<From> ) {
            if ( from < 0 ) {
                return {};
            }

            const std::make_unsigned_t<From> unsignedFrom = from;
            if ( unsignedFrom > std::numeric_limits<To>::max() ) {
                return {};
            }
        }
        // From is unsigned, To is signed
        else {
            constexpr To maxTo = std::numeric_limits<To>::max();
            static_assert( maxTo >= 0 );

            constexpr std::make_unsigned_t<To> unsignedMaxTo = maxTo;
            if ( from > unsignedMaxTo ) {
                return {};
            }
        }

        return static_cast<To>( from );
    }

    // Performs a checked conversion of a floating-point value of type From to an integer type To. Returns an empty std::optional<To>
    // if the source value does not fit into the target type.
    template <typename To, typename From,
              std::enable_if_t<( std::is_integral_v<To> && std::numeric_limits<To>::radix == 2 && std ::is_floating_point_v<From> && std::numeric_limits<From>::is_iec559
                                 && std::numeric_limits<From>::radix == 2 && std::numeric_limits<To>::digits < std::numeric_limits<From>::max_exponent ),
                               bool>
              = true>
    std::optional<To> checkedCast( const From from )
    {
        if ( !std::isfinite( from ) ) {
            return {};
        }

        static_assert( std::numeric_limits<int8_t>::min() == -128 && std::numeric_limits<int8_t>::max() == 127,
                       "The following logic will only work on platforms with two's complement signed integer representation" );

        // Value of 'from' in general case cannot be compared with std::numeric_limits<To>::max() the way it's usually done for integers due
        // to the fact that most values exceeding a certain limit cannot be exactly represented in floating-point format. For instance, when
        // converting from 'float' to 'int32_t', 'INT32_MAX' (2^31 - 1) cannot be exactly represented as 'float', because the significand of
        // 'float' is just 24 bits long (23 "real" bits + 1 "imaginary" bit), therefore, only those integers whose absolute values do not
        // exceed 2^24 can be guaranteed to be exactly represented. However, any sane integer which absolute value is 2^N can be exactly
        // represented in an IEEE 754 floating-point format, and that's what we're going to use here.
        if constexpr ( std::is_signed_v<To> ) {
            // Value of 'from' should be not less than -(2^N) and also it should be less than 2^N, where N is a number of significant
            // bits in the target type
            if ( from < std::ldexp( static_cast<From>( -1.0 ), std::numeric_limits<To>::digits )
                 || from >= std::ldexp( static_cast<From>( 1.0 ), std::numeric_limits<To>::digits ) ) {
                return {};
            }
        }
        else {
            // Value of 'from' should be not less than 0 and also it should be less than 2^N, where N is a number of significant bits
            // in the target type
            if ( from < 0 || from >= std::ldexp( static_cast<From>( 1.0 ), std::numeric_limits<To>::digits ) ) {
                return {};
            }
        }

        return static_cast<To>( from );
    }
}
