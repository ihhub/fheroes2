/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#ifndef H2MAPS_OBJECTS_H
#define H2MAPS_OBJECTS_H

#include <cstdint>
#include <initializer_list>
#include <list>
#include <string>
#include <vector>

#include "artifact.h"
#include "position.h"
#include "resource.h"

class IStreamBase;
class OStreamBase;

class MapObjectSimple : public MapPosition
{
public:
    MapObjectSimple() = default;

    uint32_t GetUID() const
    {
        return uid;
    }

    void SetUID( uint32_t v )
    {
        uid = v;
    }

    void setUIDAndIndex( const int32_t mapIndex )
    {
        SetUID( mapIndex );
        SetIndex( mapIndex );
    }

protected:
    friend OStreamBase & operator<<( OStreamBase & stream, const MapObjectSimple & obj );
    friend IStreamBase & operator>>( IStreamBase & stream, MapObjectSimple & obj );

    uint32_t uid{ 0 };
};

struct MapEvent : public MapObjectSimple
{
    MapEvent() = default;

    void LoadFromMP2( const int32_t index, const std::vector<uint8_t> & data );

    bool isAllow( const int color ) const
    {
        return ( color & colors ) != 0;
    }

    void SetVisited()
    {
        if ( isSingleTimeEvent ) {
            colors = 0;
        }
    }

    Funds resources;
    Artifact artifact;
    bool computer{ false };
    bool isSingleTimeEvent{ true };
    int colors{ 0 };
    std::string message;
};

struct MapSphinx : public MapObjectSimple
{
    MapSphinx() = default;

    void LoadFromMP2( const int32_t tileIndex, const std::vector<uint8_t> & data );

    bool isCorrectAnswer( std::string answer );

    void reset()
    {
        riddle = {};
        answers = {};

        valid = false;
        artifact = Artifact::UNKNOWN;
        resources.Reset();
    }

    void validate()
    {
        if ( artifact == Artifact::UNKNOWN && resources.GetValidItemsCount() == 0 ) {
            // No reward so nothing to ask.
            valid = false;
            return;
        }

        if ( riddle.empty() || answers.empty() ) {
            // No question or no answers then nothing to do.
            valid = false;
            return;
        }

        valid = true;
    }

    Funds resources;
    Artifact artifact;
    std::list<std::string> answers;
    std::string riddle;
    bool valid{ false };

    // This is the behavior of the original game when each answer was cut only to 4 characters.
    // This is not the case for new map format.
    bool isTruncatedAnswer{ true };
};

struct MapSign : public MapObjectSimple
{
    MapSign() = default;

    void LoadFromMP2( const int32_t mapIndex, const std::vector<uint8_t> & data );

    void setDefaultMessage();

    std::string message;
};

OStreamBase & operator<<( OStreamBase & stream, const MapEvent & obj );
IStreamBase & operator>>( IStreamBase & stream, MapEvent & obj );

OStreamBase & operator<<( OStreamBase & stream, const MapSphinx & obj );
IStreamBase & operator>>( IStreamBase & stream, MapSphinx & obj );

OStreamBase & operator<<( OStreamBase & stream, const MapSign & obj );
IStreamBase & operator>>( IStreamBase & stream, MapSign & obj );

#endif
