/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <ostream>
#include <vector>

#include "color.h"
#include "logging.h"
#include "maps_objects.h"
#include "mp2.h"
#include "rand.h"
#include "serialize.h"
#include "tools.h"
#include "translations.h"

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

void MapEvent::LoadFromMP2( int32_t index, StreamBuf st )
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
        message = st.toString();
        DEBUG_LOG( DBG_GAME, DBG_INFO,
                   "event"
                       << ": " << message )
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown id" )
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

void MapSphinx::LoadFromMP2( const int32_t tileIndex, const std::vector<uint8_t> & data )
{
    assert( data.size() >= MP2::SIZEOFMP2RIDDLE );

    // Structure containing information about a Sphinx object.
    //
    // - uint8_t (1 byte)
    //     Always 0 as an indicator that this indeed a Sphinx object.
    //
    // - uint32_t (4 bytes)
    //     The amount of Wood to get upon a successful answer.
    //
    // - uint32_t (4 bytes)
    //     The amount of Mercury to get upon a successful answer.
    //
    // - uint32_t (4 bytes)
    //     The amount of Ore to get upon a successful answer.
    //
    // - uint32_t (4 bytes)
    //     The amount of Sulfur to get upon a successful answer.
    //
    // - uint32_t (4 bytes)
    //     The amount of Crystals to get upon a successful answer.
    //
    // - uint32_t (4 bytes)
    //     The amount of Gems to get upon a successful answer.
    //
    // - uint32_t (4 bytes)
    //     The amount of Gold to get upon a successful answer.
    //
    // - uint16_t (2 bytes)
    //     An artifact to get upon a successful answer.
    //
    // - uint8_t (1 byte)
    //     A number of correct answers.
    //
    // - string of 13 bytes
    //    Null terminated string of first answer.
    //
    // - string of 13 bytes
    //    Null terminated string of second answer.
    //
    // - string of 13 bytes
    //    Null terminated string of third answer.
    //
    // - string of 13 bytes
    //    Null terminated string of forth answer.
    //
    // - string of 13 bytes
    //    Null terminated string of fifth answer.
    //
    // - string of 13 bytes
    //    Null terminated string of sixth answer.
    //
    // - string of 13 bytes
    //    Null terminated string of seventh answer.
    //
    // - string of 13 bytes
    //    Null terminated string of eighth answer.
    //
    // - string
    //    Question itself.

    StreamBuf dataStream( data );
    const uint8_t magicNumber = dataStream.get();
    if ( magicNumber != 0 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Sphinx data magic number is incorrect " << static_cast<int>( magicNumber ) )
        return;
    }

    // Retrieve the amount of resources to be given while answering the Sphinx riddle correctly.
    resources.wood = dataStream.getLE32();
    resources.mercury = dataStream.getLE32();
    resources.ore = dataStream.getLE32();
    resources.sulfur = dataStream.getLE32();
    resources.crystal = dataStream.getLE32();
    resources.gems = dataStream.getLE32();
    resources.gold = dataStream.getLE32();

    // Retrieve an artifact to be given while answering the Sphinx riddle correctly.
    artifact = dataStream.getLE16();

    uint8_t answerCount = dataStream.get();

    // Get all possible answers.
    for ( uint32_t i = 0; i < 8; ++i ) {
        std::string answer = dataStream.toString( 13 );

        if ( answerCount > 0 ) {
            --answerCount;
            if ( !answer.empty() ) {
                answers.push_back( StringLower( answer ) );
            }
        }
    }

    message = dataStream.toString();
    if ( !message.empty() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Sphinx question is empty. Marking it as an invalid object." )
        return;
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Sphinx question is '" << message << "'." )

    valid = true;

    SetIndex( tileIndex );
    SetUID( tileIndex );
}

bool MapSphinx::AnswerCorrect( const std::string & answer )
{
    const std::string ans = StringLower( answer ).substr( 0, 4 );
    auto checkAnswer = [&ans]( const std::string & str ) { return StringLower( str ).substr( 0, 4 ) == ans; };
    return std::any_of( answers.begin(), answers.end(), checkAnswer );
}

void MapSphinx::SetQuiet()
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

void MapSign::LoadFromMP2( int32_t index, StreamBuf st )
{
    st.skip( 9 );
    message = st.toString();

    if ( message.empty() ) {
        const std::vector<std::string> randomMessage{ _( "Next sign 50 miles." ), _( "Burma shave." ), _( "See Rock City." ), _( "This space for rent." ) };
        message = Rand::Get( randomMessage );
    }

    SetIndex( index );
    SetUID( index );
    DEBUG_LOG( DBG_GAME, DBG_INFO,
               "sign"
                   << ": " << message )
}

StreamBase & operator<<( StreamBase & msg, const MapSign & obj )
{
    return msg << static_cast<const MapObjectSimple &>( obj ) << obj.message;
}

StreamBase & operator>>( StreamBase & msg, MapSign & obj )
{
    return msg >> static_cast<MapObjectSimple &>( obj ) >> obj.message;
}
