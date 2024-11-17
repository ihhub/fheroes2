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

#include "maps_objects.h"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <vector>

#include "color.h"
#include "game_io.h"
#include "logging.h"
#include "mp2.h"
#include "rand.h"
#include "save_format_version.h"
#include "serialize.h"
#include "tools.h"
#include "translations.h"

void MapEvent::LoadFromMP2( const int32_t index, const std::vector<uint8_t> & data )
{
    assert( data.size() >= MP2::MP2_EVENT_STRUCTURE_MIN_SIZE );

    assert( data[0] == 1 );

    // Structure containing information about a ground event.
    //
    // - uint8_t (1 byte)
    //     Always 1 as an indicator that this indeed a ground event.
    //
    // - int32_t (4 bytes)
    //     The amount of Wood to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Mercury to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Ore to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Sulfur to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Crystals to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Gems to be given. Can be negative.
    //
    // - int32_t (4 bytes)
    //     The amount of Gold to be given. Can be negative.
    //
    // - uint16_t (2 bytes)
    //     An artifact to be given.
    //
    // - uint8_t (1 byte)
    //     A flag whether the event is applicable for AI players as well.
    //
    // - uint8_t (1 bytes)
    //     Does event occur only once?
    //
    // - unused 10 bytes
    //    Always 0.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Blue player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Green player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Red player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Yellow player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Orange player receives the event.
    //
    // - uint8_t (1 byte)
    //     A flag to determine whether Purple player receives the event.
    //
    // - string
    //    Null terminated string containing the event text.

    ROStreamBuf dataStream( data );

    dataStream.skip( 1 );

    // Get the amount of resources.
    resources.wood = static_cast<int32_t>( dataStream.getLE32() );
    resources.mercury = static_cast<int32_t>( dataStream.getLE32() );
    resources.ore = static_cast<int32_t>( dataStream.getLE32() );
    resources.sulfur = static_cast<int32_t>( dataStream.getLE32() );
    resources.crystal = static_cast<int32_t>( dataStream.getLE32() );
    resources.gems = static_cast<int32_t>( dataStream.getLE32() );
    resources.gold = static_cast<int32_t>( dataStream.getLE32() );

    // An artifact to be given.
    artifact = dataStream.getLE16() + 1;

    // The event applies to AI players as well.
    computer = ( dataStream.get() != 0 );

    // Does event occur only once?
    isSingleTimeEvent = ( dataStream.get() != 0 );

    dataStream.skip( 10 );

    colors = 0;

    if ( dataStream.get() ) {
        colors |= Color::BLUE;
    }

    if ( dataStream.get() ) {
        colors |= Color::GREEN;
    }

    if ( dataStream.get() ) {
        colors |= Color::RED;
    }

    if ( dataStream.get() ) {
        colors |= Color::YELLOW;
    }

    if ( dataStream.get() ) {
        colors |= Color::ORANGE;
    }

    if ( dataStream.get() ) {
        colors |= Color::PURPLE;
    }

    message = dataStream.getString();

    setUIDAndIndex( index );

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Ground event at tile " << index << " has event message: " << message )
}

void MapSphinx::LoadFromMP2( const int32_t tileIndex, const std::vector<uint8_t> & data )
{
    assert( data.size() >= MP2::MP2_RIDDLE_STRUCTURE_MIN_SIZE );

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

    ROStreamBuf dataStream( data );
    const uint8_t magicNumber = dataStream.get();
    if ( magicNumber != 0 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Sphinx data magic number " << static_cast<int>( magicNumber ) << " is incorrect." )
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
    artifact = dataStream.getLE16() + 1;

    uint8_t answerCount = dataStream.get();

    // Get all possible answers.
    for ( uint32_t i = 0; i < 8; ++i ) {
        const std::string answer = dataStream.getString( 13 );

        if ( answerCount > 0 ) {
            --answerCount;
            if ( !answer.empty() ) {
                answers.push_back( StringLower( answer ) );
            }
        }
    }

    riddle = dataStream.getString();
    if ( riddle.empty() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Sphinx at tile index " << tileIndex << " does not have questions. Marking it as visited." )
        return;
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Sphinx question is '" << riddle << "'." )

    setUIDAndIndex( tileIndex );
}

bool MapSphinx::isCorrectAnswer( std::string answer )
{
    if ( isTruncatedAnswer ) {
        answer = StringLower( answer ).substr( 0, 4 );
        const auto checkAnswer = [&answer]( const std::string & str ) { return StringLower( str ).substr( 0, 4 ) == answer; };
        return std::any_of( answers.begin(), answers.end(), checkAnswer );
    }

    answer = StringLower( answer );
    const auto checkAnswer = [&answer]( const std::string & str ) { return StringLower( str ) == answer; };
    return std::any_of( answers.begin(), answers.end(), checkAnswer );
}

void MapSign::LoadFromMP2( const int32_t mapIndex, const std::vector<uint8_t> & data )
{
    assert( data.size() >= MP2::MP2_SIGN_STRUCTURE_MIN_SIZE );
    assert( data[0] == 0x1 );

    // Structure containing information about a sign or bottle.
    //
    // - uint8_t (1 byte)
    //     Always equal to 1.
    //
    // - unused 8 bytes
    //    Unknown / unused. TODO: find out what these bytes used for.
    //
    // - string
    //    Null terminated string.

    ROStreamBuf dataStream( data );
    dataStream.skip( 9 );
    message = dataStream.getString();

    if ( message.empty() ) {
        setDefaultMessage();
    }

    setUIDAndIndex( mapIndex );

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Sign at location " << mapIndex << " has a message: " << message )
}

void MapSign::setDefaultMessage()
{
    const std::vector<std::string> randomMessage{ _( "Next sign 50 miles." ), _( "Burma shave." ), _( "See Rock City." ), _( "This space for rent." ) };
    message = Rand::Get( randomMessage );
}

OStreamBase & operator<<( OStreamBase & stream, const MapObjectSimple & obj )
{
    return stream << static_cast<const MapPosition &>( obj ) << obj.uid;
}

IStreamBase & operator>>( IStreamBase & stream, MapObjectSimple & obj )
{
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE2_1103_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE2_1103_RELEASE ) {
        int dummy;

        stream >> dummy >> obj.uid >> static_cast<MapPosition &>( obj );
    }
    else {
        stream >> static_cast<MapPosition &>( obj ) >> obj.uid;
    }

    return stream;
}

OStreamBase & operator<<( OStreamBase & stream, const MapEvent & obj )
{
    return stream << static_cast<const MapObjectSimple &>( obj ) << obj.resources << obj.artifact << obj.computer << obj.isSingleTimeEvent << obj.colors << obj.message;
}

IStreamBase & operator>>( IStreamBase & stream, MapEvent & obj )
{
    return stream >> static_cast<MapObjectSimple &>( obj ) >> obj.resources >> obj.artifact >> obj.computer >> obj.isSingleTimeEvent >> obj.colors >> obj.message;
}

OStreamBase & operator<<( OStreamBase & stream, const MapSphinx & obj )
{
    return stream << static_cast<const MapObjectSimple &>( obj ) << obj.resources << obj.artifact << obj.answers << obj.riddle << obj.valid << obj.isTruncatedAnswer;
}

IStreamBase & operator>>( IStreamBase & stream, MapSphinx & obj )
{
    stream >> static_cast<MapObjectSimple &>( obj ) >> obj.resources >> obj.artifact >> obj.answers >> obj.riddle >> obj.valid;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1100_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1100_RELEASE ) {
        obj.isTruncatedAnswer = true;
    }
    else {
        stream >> obj.isTruncatedAnswer;
    }

    return stream;
}

OStreamBase & operator<<( OStreamBase & stream, const MapSign & obj )
{
    return stream << static_cast<const MapObjectSimple &>( obj ) << obj.message;
}

IStreamBase & operator>>( IStreamBase & stream, MapSign & obj )
{
    return stream >> static_cast<MapObjectSimple &>( obj ) >> obj.message;
}
