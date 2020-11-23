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
#include "dialog.h"
#include "game.h"
#include "maps_objects.h"
#include "mp2.h"
#include "settings.h"

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
        computer = st.get();

        // cancel event after first visit
        cancel = st.get();

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
        DEBUG( DBG_GAME, DBG_INFO,
               "event"
                   << ": " << message );
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, "unknown id" );
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
        DEBUG( DBG_GAME, DBG_INFO,
               "sphinx"
                   << ": " << message );
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, "unknown id" );
    }
}

bool MapSphinx::AnswerCorrect( const std::string & answer )
{
    return answers.end() != std::find( answers.begin(), answers.end(), StringLower( answer ) );
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

MapSign::MapSign( s32 index, const std::string & msg )
    : MapObjectSimple( MP2::OBJ_SIGN )
{
    SetIndex( index );
    message = msg;
}

void MapSign::LoadFromMP2( s32 index, StreamBuf st )
{
    st.skip( 9 );
    message = st.toString();

    SetIndex( index );
    SetUID( index );
    message = Game::GetEncodeString( message );
    DEBUG( DBG_GAME, DBG_INFO,
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

MapResource::MapResource()
    : MapObjectSimple( MP2::OBJ_RESOURCE )
{}

StreamBase & operator<<( StreamBase & msg, const MapResource & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.resource;
}

StreamBase & operator>>( StreamBase & msg, MapResource & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.resource;
}

MapArtifact::MapArtifact()
    : MapObjectSimple( MP2::OBJ_ARTIFACT )
    , condition( 0 )
    , extended( 0 )
{}

Funds MapArtifact::QuantityFunds( void ) const
{
    switch ( condition ) {
    case 1:
        return Funds( QuantityResourceCount() );
    case 2:
        return Funds( Resource::GOLD, 2500 ) + Funds( QuantityResourceCount() );
    case 3:
        return Funds( Resource::GOLD, 3000 ) + Funds( QuantityResourceCount() );
    default:
        break;
    }

    return Funds();
}

ResourceCount MapArtifact::QuantityResourceCount( void ) const
{
    switch ( condition ) {
    case 1:
        return ResourceCount( Resource::GOLD, 2000 );
    case 2:
        return ResourceCount( extended, 3 );
    case 3:
        return ResourceCount( extended, 5 );
    default:
        break;
    }

    return ResourceCount();
}

StreamBase & operator<<( StreamBase & msg, const MapArtifact & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.artifact << obj.condition << obj.extended;
}

StreamBase & operator>>( StreamBase & msg, MapArtifact & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.artifact >> obj.condition >> obj.extended;
}

MapMonster::MapMonster()
    : MapObjectSimple( MP2::OBJ_MONSTER )
    , condition( 0 )
    , count( 0 )
{}

Troop MapMonster::QuantityTroop( void ) const
{
    return Troop( monster, count );
}

bool MapMonster::JoinConditionSkip( void ) const
{
    return Monster::JOIN_CONDITION_SKIP == condition;
}

bool MapMonster::JoinConditionMoney( void ) const
{
    return Monster::JOIN_CONDITION_MONEY == condition;
}

bool MapMonster::JoinConditionFree( void ) const
{
    return Monster::JOIN_CONDITION_FREE == condition;
}

bool MapMonster::JoinConditionForce( void ) const
{
    return Monster::JOIN_CONDITION_FORCE == condition;
}

StreamBase & operator<<( StreamBase & msg, const MapMonster & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.monster << obj.condition << obj.count;
}

StreamBase & operator>>( StreamBase & msg, MapMonster & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.monster >> obj.condition >> obj.count;
}
