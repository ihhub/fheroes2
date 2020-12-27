/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>

#include "error.h"
#include "system.h"
#include "tools.h"

enum KeyMod
{
    MOD_NONE = KMOD_NONE,
    MOD_CTRL = KMOD_CTRL,
    MOD_SHIFT = KMOD_SHIFT,
    MOD_ALT = KMOD_ALT,
    MOD_CAPS = KMOD_CAPS,
    MOD_NUM = KMOD_NUM
};

/* trim left right space */
std::string StringTrim( std::string str )
{
    if ( str.empty() )
        return str;

    std::string::iterator iter;

    // left
    iter = str.begin();
    while ( iter != str.end() && std::isspace( *iter ) )
        ++iter;
    if ( iter != str.begin() )
        str.erase( str.begin(), iter );

    if ( str.empty() )
        return str;

    // right
    iter = str.end() - 1;
    while ( iter != str.begin() && std::isspace( *iter ) )
        --iter;
    if ( iter != str.end() - 1 )
        str.erase( iter + 1, str.end() );

    return str;
}

/* convert to lower case */
std::string StringLower( std::string str )
{
    std::transform( str.begin(), str.end(), str.begin(), ::tolower );
    return str;
}

/* convert to upper case */
std::string StringUpper( std::string str )
{
    std::transform( str.begin(), str.end(), str.begin(), ::toupper );
    return str;
}

/* int to string */
std::string GetString( int value )
{
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

std::string GetStringShort( int value )
{
    if ( std::abs( value ) > 1000 ) {
        std::ostringstream stream;

        if ( std::abs( value ) > 1000000 )
            stream << value / 1000000 << "M";
        else
            stream << value / 1000 << "K";

        return stream.str();
    }

    return GetString( value );
}

std::string GetString( double value, u8 prec )
{
    std::ostringstream stream;
    stream << std::setprecision( prec ) << value;
    return stream.str();
}

std::string GetString( const Point & pt )
{
    std::ostringstream os;
    os << "point: x(" << pt.x << "), y(" << pt.y << ")";
    return os.str();
}

std::string GetString( const Size & sz )
{
    std::ostringstream os;
    os << "size: w(" << sz.w << "), h(" << sz.h << ")";
    return os.str();
}

std::string GetString( const Rect & rt )
{
    std::ostringstream os;
    os << "rect: x(" << rt.x << "), y(" << rt.y << "), w(" << rt.w << "), h(" << rt.h << ")";
    return os.str();
}

std::string GetHexString( int value, int width )
{
    std::ostringstream stream;
    stream << "0x" << std::setw( width ) << std::setfill( '0' ) << std::hex << value;
    return stream.str();
}

int CountBits( u32 val )
{
    int res = 0;

    for ( u32 itr = 0x00000001; itr; itr <<= 1 )
        if ( val & itr )
            ++res;

    return res;
}

int GetInt( const std::string & str )
{
    int res = 0;

    // decimal
    if ( str.end() == std::find_if( str.begin(), str.end(), []( const char c ) { return !std::isdigit( c ); } ) ) {
        std::istringstream ss( str );
        ss >> res;
    }
    else if ( str.size() > 2 && ( str.at( 0 ) == '+' || str.at( 0 ) == '-' )
              && str.end() == std::find_if( str.begin() + 1, str.end(), []( const char c ) { return !std::isdigit( c ); } ) ) {
        std::istringstream ss( str );
        ss >> res;
    }
    else
        // hex
        if ( str.size() > 3 && str.at( 0 ) == '0' && std::tolower( str.at( 1 ) ) == 'x'
             && str.end() == std::find_if( str.begin() + 2, str.end(), []( const char c ) { return !std::isxdigit( c ); } ) ) {
        std::istringstream ss( str );
        ss >> std::hex >> res;
    }
    else
    // str
    {
        std::string lower = StringLower( str );

        if ( lower == "on" )
            return 1;
        else if ( lower == "one" )
            return 1;
        else if ( lower == "two" )
            return 2;
        else if ( lower == "three" )
            return 3;
        else if ( lower == "four" )
            return 4;
        else if ( lower == "five" )
            return 5;
        else if ( lower == "six" )
            return 6;
        else if ( lower == "seven" )
            return 7;
        else if ( lower == "eight" )
            return 8;
        else if ( lower == "nine" )
            return 9;
    }

    return res;
}

void StringReplace( std::string & dst, const char * pred, const std::string & src )
{
    size_t pos = std::string::npos;

    while ( std::string::npos != ( pos = dst.find( pred ) ) )
        dst.replace( pos, std::strlen( pred ), src );
}

void StringReplace( std::string & dst, const char * pred, int value )
{
    StringReplace( dst, pred, GetString( value ) );
}

std::list<std::string> StringSplit( const std::string & str, const std::string & sep )
{
    std::list<std::string> list;
    size_t pos1 = 0;
    size_t pos2 = std::string::npos;

    while ( pos1 < str.size() && std::string::npos != ( pos2 = str.find( sep, pos1 ) ) ) {
        list.push_back( str.substr( pos1, pos2 - pos1 ) );
        pos1 = pos2 + sep.size();
    }

    // tail
    if ( pos1 < str.size() )
        list.push_back( str.substr( pos1, str.size() - pos1 ) );

    return list;
}

std::string InsertString( const std::string & src, size_t pos, const char * c )
{
    std::string res = src;

    if ( pos >= src.size() )
        res.append( c );
    else
        res.insert( pos, c );

    return res;
}

// from SDL_ttf
std::vector<u16> StringUTF8_to_UNICODE( const std::string & utf8 )
{
    std::vector<u16> unicode;
    unicode.reserve( utf8.size() );

    for ( std::string::const_iterator it = utf8.begin(); it < utf8.end(); ++it ) {
        u16 ch = static_cast<u8>( *it );

        if ( ch >= 0xF0 ) {
            if ( utf8.end() - it > 3 ) {
                ch = static_cast<u16>( *it++ & 0x07 ) << 18;
                ch |= static_cast<u16>( *it++ & 0x3F ) << 12;
                ch |= static_cast<u16>( *it++ & 0x3F ) << 6;
                ch |= static_cast<u16>( *it & 0x3F );
            }
            else
                break;
        }
        else if ( ch >= 0xE0 ) {
            if ( utf8.end() - it > 2 ) {
                ch = static_cast<u16>( *it++ & 0x0F ) << 12;
                ch |= static_cast<u16>( *it++ & 0x3F ) << 6;
                ch |= static_cast<u16>( *it & 0x3F );
            }
            else
                break;
        }
        else if ( ch >= 0xC0 ) {
            if ( utf8.end() - it > 1 ) {
                ch = static_cast<u16>( *it++ & 0x1F ) << 6;
                ch |= static_cast<u16>( *it & 0x3F );
            }
            else
                break;
        }

        unicode.push_back( ch );
    }

    return unicode;
}

std::string StringUNICODE_to_UTF8( const std::vector<u16> & unicode )
{
    std::string utf8;
    utf8.reserve( 2 * unicode.size() );

    for ( std::vector<u16>::const_iterator it = unicode.begin(); it != unicode.end(); ++it ) {
        if ( *it < 128 ) {
            utf8.append( 1, static_cast<char>( *it ) );
        }
        else if ( *it < 2048 ) {
            utf8.append( 1, static_cast<char>( 192 + ( ( *it - ( *it % 64 ) ) / 64 ) ) );
            utf8.append( 1, static_cast<char>( 128 + ( *it % 64 ) ) );
        }
        else {
            utf8.append( 1, static_cast<char>( 224 + ( ( *it - ( *it % 4096 ) ) / 4096 ) ) );
            utf8.append( 1, static_cast<char>( 128 + ( ( ( *it % 4096 ) - ( *it % 64 ) ) / 64 ) ) );
            utf8.append( 1, static_cast<char>( 128 + ( *it % 64 ) ) );
        }
    }

    return utf8;
}

char CharFromKeySym( KeySym sym, u16 mod )
{
    switch ( sym ) {
    case KEY_1:
        return ( MOD_SHIFT & mod ? '!' : '1' );
    case KEY_2:
        return ( MOD_SHIFT & mod ? '@' : '2' );
    case KEY_3:
        return ( MOD_SHIFT & mod ? '#' : '3' );
    case KEY_4:
        return ( MOD_SHIFT & mod ? '$' : '4' );
    case KEY_5:
        return ( MOD_SHIFT & mod ? '%' : '5' );
    case KEY_6:
        return ( MOD_SHIFT & mod ? '^' : '6' );
    case KEY_7:
        return ( MOD_SHIFT & mod ? '&' : '7' );
    case KEY_8:
        return ( MOD_SHIFT & mod ? '*' : '8' );
    case KEY_9:
        return ( MOD_SHIFT & mod ? '(' : '9' );
    case KEY_0:
        return ( MOD_SHIFT & mod ? ')' : '0' );

    case KEY_KP0:
        if ( MOD_NUM & mod )
            return '0';
        break;
    case KEY_KP1:
        if ( MOD_NUM & mod )
            return '1';
        break;
    case KEY_KP2:
        if ( MOD_NUM & mod )
            return '2';
        break;
    case KEY_KP3:
        if ( MOD_NUM & mod )
            return '3';
        break;
    case KEY_KP4:
        if ( MOD_NUM & mod )
            return '4';
        break;
    case KEY_KP5:
        if ( MOD_NUM & mod )
            return '5';
        break;
    case KEY_KP6:
        if ( MOD_NUM & mod )
            return '6';
        break;
    case KEY_KP7:
        if ( MOD_NUM & mod )
            return '7';
        break;
    case KEY_KP8:
        if ( MOD_NUM & mod )
            return '8';
        break;
    case KEY_KP9:
        if ( MOD_NUM & mod )
            return '9';
        break;

    case KEY_MINUS:
        return ( MOD_SHIFT & mod ? '_' : '-' );
    case KEY_EQUALS:
        return ( MOD_SHIFT & mod ? '+' : '=' );
    case KEY_BACKSLASH:
        return ( MOD_SHIFT & mod ? '|' : '\\' );
    case KEY_LEFTBRACKET:
        return ( MOD_SHIFT & mod ? '{' : '[' );
    case KEY_RIGHTBRACKET:
        return ( MOD_SHIFT & mod ? '}' : ']' );
    case KEY_SEMICOLON:
        return ( MOD_SHIFT & mod ? ':' : ';' );
    case KEY_QUOTE:
        return ( MOD_SHIFT & mod ? '"' : '\'' );
    case KEY_COMMA:
        return ( MOD_SHIFT & mod ? '<' : ',' );
    case KEY_PERIOD:
        return ( MOD_SHIFT & mod ? '>' : '.' );
    case KEY_SLASH:
        return ( MOD_SHIFT & mod ? '?' : '/' );

    case KEY_EXCLAIM:
        return '!';
    case KEY_AT:
        return '@';
    case KEY_HASH:
        return '#';
    case KEY_DOLLAR:
        return '$';
    case KEY_AMPERSAND:
        return '&';
    case KEY_ASTERISK:
        return '*';
    case KEY_LEFTPAREN:
        return '(';
    case KEY_RIGHTPAREN:
        return ')';
    case KEY_QUOTEDBL:
        return '"';
    case KEY_PLUS:
        return '+';
    case KEY_COLON:
        return ':';
    case KEY_LESS:
        return '<';
    case KEY_GREATER:
        return '>';
    case KEY_QUESTION:
        return '?';
    case KEY_CARET:
        return '^';
    case KEY_UNDERSCORE:
        return '_';

    case KEY_SPACE:
        return ' ';

    case KEY_a:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'A' : 'a' );
    case KEY_b:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'B' : 'b' );
    case KEY_c:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'C' : 'c' );
    case KEY_d:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'D' : 'd' );
    case KEY_e:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'E' : 'e' );
    case KEY_f:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'F' : 'f' );
    case KEY_g:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'G' : 'g' );
    case KEY_h:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'H' : 'h' );
    case KEY_i:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'I' : 'i' );
    case KEY_j:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'J' : 'j' );
    case KEY_k:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'K' : 'k' );
    case KEY_l:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'L' : 'l' );
    case KEY_m:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'M' : 'm' );
    case KEY_n:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'N' : 'n' );
    case KEY_o:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'O' : 'o' );
    case KEY_p:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'P' : 'p' );
    case KEY_q:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'Q' : 'q' );
    case KEY_r:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'R' : 'r' );
    case KEY_s:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'S' : 's' );
    case KEY_t:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'T' : 't' );
    case KEY_u:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'U' : 'u' );
    case KEY_v:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'V' : 'v' );
    case KEY_w:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'W' : 'w' );
    case KEY_x:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'X' : 'x' );
    case KEY_y:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'Y' : 'y' );
    case KEY_z:
        return ( ( MOD_SHIFT | MOD_CAPS ) & mod ? 'Z' : 'z' );

    default:
        break;
    }

    return 0;
}

size_t InsertKeySym( std::string & res, size_t pos, KeySym sym, u16 mod )
{
    switch ( sym ) {
    case KEY_BACKSPACE: {
        if ( res.size() && pos ) {
            if ( pos >= res.size() )
                res.resize( res.size() - 1 );
            else
                res.erase( pos - 1, 1 );
            --pos;
        }
    } break;
    case KEY_DELETE: {
        if ( res.size() ) {
            if ( pos < res.size() )
                res.erase( pos, 1 );
        }
    } break;

    case KEY_LEFT:
        if ( pos )
            --pos;
        break;
    case KEY_RIGHT:
        if ( pos < res.size() )
            ++pos;
        break;

    default: {
        char c = CharFromKeySym( sym, mod );

        if ( c ) {
            res.insert( pos, 1, c );
            ++pos;
        }
    }
    }

    return pos;
}

int Sign( int s )
{
    return ( s < 0 ? -1 : ( s > 0 ? 1 : 0 ) );
}

KeySym KeySymFromChar( char c )
{
    switch ( c ) {
    case '!':
        return KEY_EXCLAIM;
    case '"':
        return KEY_QUOTEDBL;
    case '#':
        return KEY_HASH;
    case '$':
        return KEY_DOLLAR;
    case '&':
        return KEY_AMPERSAND;
    case '\'':
        return KEY_QUOTE;
    case '(':
        return KEY_LEFTPAREN;
    case ')':
        return KEY_RIGHTPAREN;
    case '*':
        return KEY_ASTERISK;
    case '+':
        return KEY_PLUS;
    case ',':
        return KEY_COMMA;
    case '-':
        return KEY_MINUS;
    case '.':
        return KEY_PERIOD;
    case '/':
        return KEY_SLASH;
    case ':':
        return KEY_COLON;
    case ';':
        return KEY_SEMICOLON;
    case '<':
        return KEY_LESS;
    case '=':
        return KEY_EQUALS;
    case '>':
        return KEY_GREATER;
    case '?':
        return KEY_QUESTION;
    case '@':
        return KEY_AT;
    case '[':
        return KEY_LEFTBRACKET;
    case '\\':
        return KEY_BACKSLASH;
    case ']':
        return KEY_RIGHTBRACKET;
    case '^':
        return KEY_CARET;
    case '_':
        return KEY_UNDERSCORE;
    case ' ':
        return KEY_SPACE;

    case 'a':
        return KEY_a;
    case 'b':
        return KEY_b;
    case 'c':
        return KEY_c;
    case 'd':
        return KEY_d;
    case 'e':
        return KEY_e;
    case 'f':
        return KEY_f;
    case 'g':
        return KEY_g;
    case 'h':
        return KEY_h;
    case 'i':
        return KEY_i;
    case 'j':
        return KEY_j;
    case 'k':
        return KEY_k;
    case 'l':
        return KEY_l;
    case 'm':
        return KEY_m;
    case 'n':
        return KEY_n;
    case 'o':
        return KEY_o;
    case 'p':
        return KEY_p;
    case 'q':
        return KEY_q;
    case 'r':
        return KEY_r;
    case 's':
        return KEY_s;
    case 't':
        return KEY_t;
    case 'u':
        return KEY_u;
    case 'v':
        return KEY_v;
    case 'w':
        return KEY_w;
    case 'x':
        return KEY_x;
    case 'y':
        return KEY_y;
    case 'z':
        return KEY_z;

    case '0':
        return KEY_0;
    case '1':
        return KEY_1;
    case '2':
        return KEY_2;
    case '3':
        return KEY_3;
    case '4':
        return KEY_4;
    case '5':
        return KEY_5;
    case '6':
        return KEY_6;
    case '7':
        return KEY_7;
    case '8':
        return KEY_8;
    case '9':
        return KEY_9;

    default:
        break;
    }
    return KEY_NONE;
}

bool SaveMemToFile( const std::vector<u8> & data, const std::string & file )
{
    SDL_RWops * rw = SDL_RWFromFile( file.c_str(), "wb" );

    if ( rw && 1 == SDL_RWwrite( rw, &data[0], data.size(), 1 ) )
        SDL_RWclose( rw );
    else {
        ERROR( SDL_GetError() );
        return false;
    }

    return true;
}

std::vector<u8> LoadFileToMem( const std::string & file )
{
    std::vector<u8> data;
    SDL_RWops * rw = SDL_RWFromFile( file.c_str(), "rb" );
    if ( rw == NULL )
        ERROR( SDL_GetError() );

    const Sint64 length = SDL_RWseek( rw, 0, RW_SEEK_END );
    if ( length < 0 )
        ERROR( SDL_GetError() );

    if ( length > 0 ) {
        data.resize( length );
        SDL_RWseek( rw, 0, RW_SEEK_SET );
        SDL_RWread( rw, &data[0], data.size(), 1 );
    }

    SDL_RWclose( rw );

    return data;
}

bool PressIntKey( u32 max, u32 & result )
{
    LocalEvent & le = LocalEvent::Get();

    if ( le.KeyPress( KEY_BACKSPACE ) ) {
        result /= 10;
        return true;
    }
    else if ( le.KeyPress() && KEY_0 <= le.KeyValue() && KEY_9 >= le.KeyValue() ) {
        if ( max > result ) {
            result *= 10;
            switch ( le.KeyValue() ) {
            case KEY_1:
                result += 1;
                break;
            case KEY_2:
                result += 2;
                break;
            case KEY_3:
                result += 3;
                break;
            case KEY_4:
                result += 4;
                break;
            case KEY_5:
                result += 5;
                break;
            case KEY_6:
                result += 6;
                break;
            case KEY_7:
                result += 7;
                break;
            case KEY_8:
                result += 8;
                break;
            case KEY_9:
                result += 9;
                break;

            case KEY_KP1:
                result += 1;
                break;
            case KEY_KP2:
                result += 2;
                break;
            case KEY_KP3:
                result += 3;
                break;
            case KEY_KP4:
                result += 4;
                break;
            case KEY_KP5:
                result += 5;
                break;
            case KEY_KP6:
                result += 6;
                break;
            case KEY_KP7:
                result += 7;
                break;
            case KEY_KP8:
                result += 8;
                break;
            case KEY_KP9:
                result += 9;
                break;

            default:
                break;
            }
            if ( result > max )
                result = max;
        }
        return true;
    }
    return false;
}

#ifdef WITH_ICONV
#include <iconv.h>
std::string EncodeString( const std::string & str, const char * charset )
{
    iconv_t cd;

    if ( !charset || ( iconv_t )( -1 ) == ( cd = iconv_open( "utf-8", charset ) ) )
        return str;

    std::string res( str );
    size_t inbytesleft = str.size();
    size_t outbytesleft = inbytesleft * 2 + 1;
    const char * inbuf = str.c_str();
    char * outbuf1 = new char[outbytesleft];
    char * outbuf2 = outbuf1;

    size_t reslen = iconv( cd, const_cast<char **>( &inbuf ), &inbytesleft, &outbuf1, &outbytesleft );

    iconv_close( cd );

    if ( reslen != ( size_t )( -1 ) )
        res = std::string( outbuf2, outbuf1 - outbuf2 );

    delete[] outbuf2;

    return res;
}
#else
std::string cp1251_to_utf8( const std::string & in )
{
    const u32 table_1251[]
        = {0x82D0, 0x83D0,   0x9A80E2, 0x93D1,   0x9E80E2, 0xA680E2, 0xA080E2, 0xA180E2, 0xAC82E2, 0xB080E2, 0x89D0, 0xB980E2, 0x8AD0, 0x8CD0, 0x8BD0, 0x8FD0,
           0x92D1, 0x9880E2, 0x9980E2, 0x9C80E2, 0x9D80E2, 0xA280E2, 0x9380E2, 0x9480E2, 0,        0xA284E2, 0x99D1, 0xBA80E2, 0x9AD1, 0x9CD1, 0x9BD1, 0x9FD1,
           0xA0C2, 0x8ED0,   0x9ED1,   0x88D0,   0xA4C2,   0x90D2,   0xA6C2,   0xA7C2,   0x81D0,   0xA9C2,   0x84D0, 0xABC2,   0xACC2, 0xADC2, 0xAEC2, 0x87D0,
           0xB0C2, 0xB1C2,   0x86D0,   0x96D1,   0x91D2,   0xB5C2,   0xB6C2,   0xB7C2,   0x91D1,   0x9684E2, 0x94D1, 0xBBC2,   0x98D1, 0x85D0, 0x95D1, 0x97D1,
           0x90D0, 0x91D0,   0x92D0,   0x93D0,   0x94D0,   0x95D0,   0x96D0,   0x97D0,   0x98D0,   0x99D0,   0x9AD0, 0x9BD0,   0x9CD0, 0x9DD0, 0x9ED0, 0x9FD0,
           0xA0D0, 0xA1D0,   0xA2D0,   0xA3D0,   0xA4D0,   0xA5D0,   0xA6D0,   0xA7D0,   0xA8D0,   0xA9D0,   0xAAD0, 0xABD0,   0xACD0, 0xADD0, 0xAED0, 0xAFD0,
           0xB0D0, 0xB1D0,   0xB2D0,   0xB3D0,   0xB4D0,   0xB5D0,   0xB6D0,   0xB7D0,   0xB8D0,   0xB9D0,   0xBAD0, 0xBBD0,   0xBCD0, 0xBDD0, 0xBED0, 0xBFD0,
           0x80D1, 0x81D1,   0x82D1,   0x83D1,   0x84D1,   0x85D1,   0x86D1,   0x87D1,   0x88D1,   0x89D1,   0x8AD1, 0x8BD1,   0x8CD1, 0x8DD1, 0x8ED1, 0x8FD1};

    std::string res;
    res.reserve( in.size() * 2 + 1 );

    for ( std::string::const_iterator it = in.begin(); it != in.end(); ++it ) {
        if ( *it & 0x80 ) {
            const size_t index = *it & 0x7f;

            if ( index < ARRAY_COUNT( table_1251 ) ) {
                const uint32_t v = table_1251[index];
                res.append( 1, v );
                res.append( 1, v >> 8 );
                if ( v & 0xFFFF0000 )
                    res.append( 1, v >> 16 );
            }
        }
        else
            res.append( 1, *it );
    }

    return res;
}

std::string EncodeString( const std::string & str, const char * charset )
{
    if ( charset ) {
        if ( 0 == std::strcmp( charset, "cp1251" ) )
            return cp1251_to_utf8( str );
    }

    return str;
}
#endif

double GetAngle( const Point & start, const Point & target )
{
    const int dx = target.x - start.x;
    const int dy = target.y - start.y;
    double angle = atan2( -dy, dx ) * 180.0 / M_PI;
    // we only care about two quadrants, normalize
    if ( dx < 0 ) {
        angle = ( dy <= 0 ) ? 180 - angle : -angle - 180;
    }
    return angle;
}

Points GetEuclideanLine( const Point & pt1, const Point & pt2, u16 step )
{
    const int dx = pt2.x - pt1.x;
    const int dy = pt2.y - pt1.y;
    const uint32_t dist = hypot( std::abs( dx ), std::abs( dy ) );
    // round up the integer division
    const uint32_t length = ( step > 0 ) ? ( dist + step / 2 ) / step : 1;
    const double moveX = dx / static_cast<double>( length );
    const double moveY = dy / static_cast<double>( length );

    Points line;
    line.reserve( length );

    for ( uint32_t i = 0; i <= length; ++i ) {
        line.push_back( Point( static_cast<int>( pt1.x + i * moveX ), static_cast<int>( pt1.y + i * moveY ) ) );
    }

    return line;
}

Points GetLinePoints( const Point & pt1, const Point & pt2, u16 step )
{
    Points res;
    res.reserve( 10 );

    const u16 dx = std::abs( pt2.x - pt1.x );
    const u16 dy = std::abs( pt2.y - pt1.y );

    int16_t ns = std::div( ( dx > dy ? dx : dy ), 2 ).quot;
    Point pt( pt1 );

    for ( u16 i = 0; i <= ( dx > dy ? dx : dy ); ++i ) {
        if ( dx > dy ) {
            pt.x < pt2.x ? ++pt.x : --pt.x;
            ns -= dy;
        }
        else {
            pt.y < pt2.y ? ++pt.y : --pt.y;
            ns -= dx;
        }

        if ( ns < 0 ) {
            if ( dx > dy ) {
                pt.y < pt2.y ? ++pt.y : --pt.y;
                ns += dx;
            }
            else {
                pt.x < pt2.x ? ++pt.x : --pt.x;
                ns += dy;
            }
        }

        if ( 0 == ( i % step ) )
            res.push_back( pt );
    }

    return res;
}

Points GetArcPoints( const Point & from, const Point & to, const Point & max, u16 step )
{
    Points res;
    Point pt1, pt2;

    pt1 = from;
    pt2 = Point( from.x + std::abs( max.x - from.x ) / 2, from.y - std::abs( max.y - from.y ) * 3 / 4 );
    const Points & pts1 = GetLinePoints( pt1, pt2, step );
    res.insert( res.end(), pts1.begin(), pts1.end() );

    pt1 = pt2;
    pt2 = max;
    const Points & pts2 = GetLinePoints( pt1, pt2, step );
    res.insert( res.end(), pts2.begin(), pts2.end() );

    pt1 = max;
    pt2 = Point( max.x + std::abs( to.x - max.x ) / 2, to.y - std::abs( to.y - max.y ) * 3 / 4 );
    const Points & pts3 = GetLinePoints( pt1, pt2, step );
    res.insert( res.end(), pts3.begin(), pts3.end() );

    pt1 = pt2;
    pt2 = to;
    const Points & pts4 = GetLinePoints( pt1, pt2, step );
    res.insert( res.end(), pts4.begin(), pts4.end() );

    return res;
}

u32 decodeChar( int v )
{
    if ( 'A' <= v && v <= 'Z' )
        return v - 'A';

    if ( 'a' <= v && v <= 'z' )
        return v - 'a' + 26;

    if ( '0' <= v && v <= '9' )
        return v - '0' + 52;

    if ( v == '+' )
        return 62;

    if ( v == '/' )
        return 63;

    return 0;
}

std::vector<u8> decodeBase64( const std::string & src )
{
    std::vector<u8> res;

    if ( src.size() % 4 == 0 ) {
        u32 size = 3 * src.size() / 4;

        if ( src[src.size() - 1] == '=' )
            size--;
        if ( src[src.size() - 2] == '=' )
            size--;

        res.reserve( size );

        for ( u32 ii = 0; ii < src.size(); ii += 4 ) {
            u32 sextet_a = decodeChar( src[ii] );
            u32 sextet_b = decodeChar( src[ii + 1] );
            u32 sextet_c = decodeChar( src[ii + 2] );
            u32 sextet_d = decodeChar( src[ii + 3] );

            u32 triple = ( sextet_a << 18 ) + ( sextet_b << 12 ) + ( sextet_c << 6 ) + sextet_d;

            if ( res.size() < size )
                res.push_back( ( triple >> 16 ) & 0xFF );
            if ( res.size() < size )
                res.push_back( ( triple >> 8 ) & 0xFF );
            if ( res.size() < size )
                res.push_back( triple & 0xFF );
        }
    }

    return res;
}

int CheckSum( const std::vector<u8> & v )
{
    u32 ret = 0;
    std::vector<u8>::const_iterator it = v.begin();

    do {
        u32 b1 = it < v.end() ? *it++ : 0;
        u32 b2 = it < v.end() ? *it++ : 0;
        u32 b3 = it < v.end() ? *it++ : 0;
        u32 b4 = it < v.end() ? *it++ : 0;

        ret += ( b1 << 24 ) | ( b2 << 16 ) | ( b3 << 8 ) | b4;
    } while ( it != v.end() );

    return ret;
}

int CheckSum( const std::string & str )
{
    return CheckSum( std::vector<u8>( str.begin(), str.end() ) );
}
