/***************************************************************************
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "army_troop.h"
#include "color.h"
#include "game.h"
#include "logging.h"
#include "maps_objects.h"
#include "mp2.h"
#include "tools.h"

#define SIZEMESSAGE 400

StreamBase & operator<<( StreamBase & msg, const MapObjectSimple & obj )
{
    return msg << obj.type << obj.uid << static_cast<const MapPosition &>( obj );
}

StreamBase & operator>>( StreamBase & msg, MapObjectSimple & obj )
{
    return msg >> obj.type >> obj.uid >> static_cast<MapPosition &>( obj );
}

MapEvent::MapEvent()
    : MapObjectSimple( MP2::OBJ_EVENT )
    , computer( false )
    , cancel( true )
    , colors( 0 )
{}

void MapEvent::LoadFromMP2( s32 index, StreamBuf st )
{
    // id
    if ( 1 == st.get() ) {
        SetIndex( index );
        SetUID( index );

        // resource
        resources.wood = st.getLE32();
        resources.mercury = st.getLE32();
        resources.ore = st.getLE32();
        resources.sulfur = st.getLE32();
        resources.crystal = st.getLE32();
        resources.gems = st.getLE32();
        resources.gold = st.getLE32();

        // artifact
        artifact = st.getLE16();

        // allow computer
        computer = ( st.get() != 0 );

        // cancel event after first visit
        cancel = ( st.get() != 0 );

        st.skip( 10 );

        colors = 0;
        // blue
        if ( st.get() )
            colors |= Color::BLUE;
        // green
        if ( st.get() )
            colors |= Color::GREEN;
        // red
        if ( st.get() )
            colors |= Color::RED;
        // yellow
        if ( st.get() )
            colors |= Color::YELLOW;
        // orange
        if ( st.get() )
            colors |= Color::ORANGE;
        // purple
        if ( st.get() )
            colors |= Color::PURPLE;

        // message
        message = Game::GetEncodeString( st.toString() );
        DEBUG_LOG( DBG_GAME, DBG_INFO,
                   "event"
                       << ": " << message );
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown id" );
    }
}

void MapEvent::SetVisited( int color )
{
    if ( cancel )
        colors = 0;
    else
        colors &= ~color;
}

bool MapEvent::isAllow( int col ) const
{
    return ( col & colors ) != 0;
}

MapSphinx::MapSphinx()
    : MapObjectSimple( MP2::OBJ_SPHINX )
    , valid( false )
{}

void MapSphinx::LoadFromMP2( s32 index, StreamBuf st )
{
    // id
    if ( 0 == st.get() ) {
        SetIndex( index );
        SetUID( index );

        // resource
        resources.wood = st.getLE32();
        resources.mercury = st.getLE32();
        resources.ore = st.getLE32();
        resources.sulfur = st.getLE32();
        resources.crystal = st.getLE32();
        resources.gems = st.getLE32();
        resources.gold = st.getLE32();

        // artifact
        artifact = st.getLE16();

        // count answers
        u32 count = st.get();

        // answers
        for ( u32 i = 0; i < 8; ++i ) {
            std::string answer = Game::GetEncodeString( st.toString( 13 ) );

            if ( count-- && answer.size() )
                answers.push_back( StringLower( answer ) );
        }

        // message
        message = Game::GetEncodeString( st.toString() );

        valid = true;
        DEBUG_LOG( DBG_GAME, DBG_INFO,
                   "sphinx"
                       << ": " << message );
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown id" );
    }
}

bool MapSphinx::AnswerCorrect( const std::string & answer )
{
    const std::string ans = StringLower( answer ).substr( 0, 4 );
    auto checkAnswer = [ans]( const std::string & str ) { return StringLower( str ).substr( 0, 4 ) == ans; };
    return std::any_of( answers.begin(), answers.end(), checkAnswer );
}

void MapSphinx::SetQuiet( void )
{
    valid = false;
    artifact = Artifact::UNKNOWN;
    resources.Reset();
}

StreamBase & operator<<( StreamBase & msg, const MapEvent & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.resources << obj.artifact << obj.computer << obj.cancel << obj.colors << obj.message;
}

StreamBase & operator>>( StreamBase & msg, MapEvent & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.resources >> obj.artifact >> obj.computer >> obj.cancel >> obj.colors >> obj.message;
}

StreamBase & operator<<( StreamBase & msg, const MapSphinx & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.resources << obj.artifact << obj.answers << obj.message << obj.valid;
}

StreamBase & operator>>( StreamBase & msg, MapSphinx & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.resources >> obj.artifact >> obj.answers >> obj.message >> obj.valid;
}

MapSign::MapSign()
    : MapObjectSimple( MP2::OBJ_SIGN )
{}

void MapSign::LoadFromMP2( s32 index, StreamBuf st )
{
    st.skip( 9 );
    message = st.toString();

    SetIndex( index );
    SetUID( index );
    message = Game::GetEncodeString( message );
    DEBUG_LOG( DBG_GAME, DBG_INFO,
               "sign"
                   << ": " << message );
}

StreamBase & operator<<( StreamBase & msg, const MapSign & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.message;
}

StreamBase & operator>>( StreamBase & msg, MapSign & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.message;
}
