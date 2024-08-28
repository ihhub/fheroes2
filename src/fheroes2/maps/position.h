/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2POSITION_H
#define H2POSITION_H

#include <cstdint>

#include "math_base.h"

class IStreamBase;
class OStreamBase;

class MapPosition
{
public:
    explicit MapPosition( const fheroes2::Point & = fheroes2::Point( -1, -1 ) );
    MapPosition( const MapPosition & ) = delete;

    virtual ~MapPosition() = default;

    MapPosition & operator=( const MapPosition & ) = delete;

    const fheroes2::Point & GetCenter() const
    {
        return center;
    }

    int32_t GetIndex() const;

    void SetCenter( const fheroes2::Point & );
    void SetIndex( const int32_t index );

    virtual bool isPosition( const fheroes2::Point & pt ) const
    {
        return pt == center;
    }

protected:
    friend OStreamBase & operator<<( OStreamBase & stream, const MapPosition & st );
    friend IStreamBase & operator>>( IStreamBase & stream, MapPosition & st );

    fheroes2::Point center;
};

#endif
