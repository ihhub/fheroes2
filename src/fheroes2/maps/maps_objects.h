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
#include <list>
#include <string>
#include <vector>

#include "artifact.h"
#include "mp2.h"
#include "position.h"
#include "resource.h"

class StreamBase;

class MapObjectSimple : public MapPosition
{
public:
    explicit MapObjectSimple( const int objectType = MP2::OBJ_NONE )
        : uid( 0 )
        , type( objectType )
    {
        // Do nothing.
    }

    ~MapObjectSimple() override = default;

    int GetType() const
    {
        return type;
    }

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
    friend StreamBase & operator<<( StreamBase & msg, const MapObjectSimple & obj );
    friend StreamBase & operator>>( StreamBase & msg, MapObjectSimple & obj );

    uint32_t uid;
    int type;
};

struct MapEvent : public MapObjectSimple
{
    MapEvent()
        : MapObjectSimple( MP2::OBJ_EVENT )
    {
        // Do nothing.
    }

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
    MapSphinx()
        : MapObjectSimple( MP2::OBJ_SPHINX )
    {
        // Do nothing.
    }

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
    MapSign()
        : MapObjectSimple( MP2::OBJ_SIGN )
    {
        // Do nothing.
    }

    void LoadFromMP2( const int32_t mapIndex, const std::vector<uint8_t> & data );

    void setDefaultMessage();

    std::string message;
};

StreamBase & operator<<( StreamBase & msg, const MapObjectSimple & obj );
StreamBase & operator>>( StreamBase & msg, MapObjectSimple & obj );

StreamBase & operator<<( StreamBase & msg, const MapEvent & obj );
StreamBase & operator>>( StreamBase & msg, MapEvent & obj );

StreamBase & operator<<( StreamBase & msg, const MapSphinx & obj );
StreamBase & operator>>( StreamBase & msg, MapSphinx & obj );

StreamBase & operator<<( StreamBase & msg, const MapSign & obj );
StreamBase & operator>>( StreamBase & msg, MapSign & obj );

#endif
