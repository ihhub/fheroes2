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
#ifndef H2TOOLS_H
#define H2TOOLS_H

#include <list>
#include <string>

#include "rect.h"
#include "types.h"

std::string GetStringShort( int );
std::string GetHexString( int value, int width = 8 );

int GetInt( const std::string & );
int Sign( int );

std::string StringTrim( std::string );
std::string StringLower( std::string );
std::string StringUpper( std::string );

std::list<std::string> StringSplit( const std::string &, const std::string & );

void StringReplace( std::string &, const char *, const std::string & );
void StringReplace( std::string &, const char *, int );

int CountBits( u32 );
int CheckSum( const std::vector<u8> & );
int CheckSum( const std::string & );

std::string EncodeString( const std::string &, const char * charset );
std::vector<u16> StringUTF8_to_UNICODE( const std::string & );
std::string StringUNICODE_to_UTF8( const std::vector<u16> & );

std::vector<u8> decodeBase64( const std::string & );

std::string InsertString( const std::string &, size_t, const char * );

bool SaveMemToFile( const std::vector<u8> &, const std::string & );
std::vector<u8> LoadFileToMem( const std::string & );

double GetAngle( const Point & start, const Point & target );
Points GetEuclideanLine( const Point & pt1, const Point & pt2, u16 step );
Points GetLinePoints( const Point & pt1, const Point & pt2, u16 step );
Points GetArcPoints( const Point & from, const Point & to, const Point & max, u16 step );

// std::clamp replacement until we can use C++17
template <typename T>
T clamp( const T & value, const T & min, const T & max )
{
    return ( value < min ) ? min : ( max < value ) ? max : value;
}

#endif
