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
#include <fstream>
#include <iomanip>
#include <list>
#include <utility>
#include <vector>

#include "audio_music.h"
#include "engine.h"
#include "system.h"

#define TAG_FORM 0x464F524D
#define TAG_XDIR 0x58444952
#define TAG_INFO 0x494E464F
#define TAG_CAT0 0x43415420
#define TAG_XMID 0x584D4944
#define TAG_TIMB 0x54494D42
#define TAG_EVNT 0x45564E54
#define TAG_RBRN 0x5242524E
#define TAG_MTHD 0x4D546864
#define TAG_MTRK 0x4D54726B

// Pair: time and length
struct XMI_Time : public std::pair<u32, u32>
{
    XMI_Time()
        : std::pair<u32, u32>( 0, 0 )
    {}
};

XMI_Time readXMITime( const uint8_t * data )
{
    const u8 * p = data;
    XMI_Time res;

    while ( *p & 0x80 ) {
        if ( 4 <= p - data ) {
            ERROR( "Can't read XMI time: field bigger than 4 bytes" );
            break;
        }

        res.first |= 0x0000007F & *p;
        res.first <<= 7;
        ++p;
    }

    res.first += *p;
    res.second = p - data + 1;

    return res;
}

std::vector<u8> packToMIDITime( u32 delta )
{
    u8 c1 = delta & 0x0000007F;
    u8 c2 = ( delta & 0x00003F80 ) >> 7;
    u8 c3 = ( delta & 0x001FC000 ) >> 14;
    u8 c4 = ( delta & 0x0FE00000 ) >> 21;

    std::vector<u8> res;
    res.reserve( 4 );

    if ( c4 ) {
        res.push_back( c4 | 0x80 );
        res.push_back( c3 | 0x80 );
        res.push_back( c2 | 0x80 );
        res.push_back( c1 );
    }
    else if ( c3 ) {
        res.push_back( c3 | 0x80 );
        res.push_back( c2 | 0x80 );
        res.push_back( c1 );
    }
    else if ( c2 ) {
        res.push_back( c2 | 0x80 );
        res.push_back( c1 );
    }
    else
        res.push_back( c1 );

    return res;
}

struct IFFChunkHeader
{
    u32 ID; // 4 upper case ASCII chars, padded with 0x20 (space)
    u32 length; // big-endian

    IFFChunkHeader( u32 id, u32 sz )
        : ID( id )
        , length( sz )
    {}
    IFFChunkHeader()
        : ID( 0 )
        , length( 0 )
    {}
};

StreamBuf & operator>>( StreamBuf & sb, IFFChunkHeader & st )
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    return sb;
}

StreamBuf & operator<<( StreamBuf & sb, const IFFChunkHeader & st )
{
    sb.putBE32( st.ID );
    sb.putBE32( st.length );
    return sb;
}

struct GroupChunkHeader
{
    u32 ID; // 4 byte ASCII string, either 'FORM', 'CAT ' or 'LIST'
    u32 length;
    u32 type; // 4 byte ASCII string

    GroupChunkHeader( u32 id, u32 sz, u32 tp )
        : ID( id )
        , length( sz )
        , type( tp )
    {}
    GroupChunkHeader()
        : ID( 0 )
        , length( 0 )
        , type( 0 )
    {}
};

StreamBuf & operator<<( StreamBuf & sb, const GroupChunkHeader & st )
{
    sb.putBE32( st.ID );
    sb.putBE32( st.length );
    sb.putBE32( st.type );
    return sb;
}

StreamBuf & operator>>( StreamBuf & sb, GroupChunkHeader & st )
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    st.type = sb.getBE32();
    return sb;
}

struct XMITrack
{
    std::vector<u8> timb;
    std::vector<u8> evnt;
};

struct XMITracks : std::list<XMITrack>
{};

struct XMIData
{
    XMITracks tracks;

    XMIData( const std::vector<u8> & buf )
    {
        StreamBuf sb( buf );

        GroupChunkHeader group;
        IFFChunkHeader iff;

        // FORM XDIR
        sb >> group;
        if ( group.ID == TAG_FORM && group.type == TAG_XDIR ) {
            // INFO
            sb >> iff;
            if ( iff.ID == TAG_INFO && iff.length == 2 ) {
                int numTracks = sb.getLE16();

                // CAT XMID
                sb >> group;
                if ( group.ID == TAG_CAT0 && group.type == TAG_XMID ) {
                    for ( int track = 0; track < numTracks; ++track ) {
                        tracks.push_back( XMITrack() );

                        std::vector<u8> & timb = tracks.back().timb;
                        std::vector<u8> & evnt = tracks.back().evnt;

                        sb >> group;
                        // FORM XMID
                        if ( group.ID == TAG_FORM && group.type == TAG_XMID ) {
                            sb >> iff;
                            // [TIMB]
                            if ( iff.ID == TAG_TIMB ) {
                                timb = sb.getRaw( iff.length );
                                if ( timb.size() != iff.length ) {
                                    ERROR( "parse error: "
                                           << "out of range" );
                                    break;
                                }
                                sb >> iff;
                            }

                            // [RBRN]
                            if ( iff.ID == TAG_RBRN ) {
                                sb.skip( iff.length );
                                sb >> iff;
                            }

                            // EVNT
                            if ( iff.ID != TAG_EVNT ) {
                                ERROR( "parse error: "
                                       << "evnt" );
                                break;
                            }

                            evnt = sb.getRaw( iff.length );

                            if ( evnt.size() != iff.length ) {
                                ERROR( "parse error: "
                                       << "out of range" );
                                break;
                            }
                        }
                        else
                            ERROR( "unknown tag: " << group.ID << " (expected FORM), " << group.type << " (expected XMID)" );
                    }
                }
                else
                    ERROR( "parse error: "
                           << "cat xmid" );
            }
            else
                ERROR( "parse error: "
                       << "info" );
        }
        else
            ERROR( "parse error: "
                   << "form xdir" );
    }

    bool isvalid( void ) const
    {
        return !tracks.empty();
    }
};

struct MidiChunk
{
    uint32_t _time;
    uint8_t _type;
    std::vector<uint8_t> _binaryTime;
    std::vector<uint8_t> _data;

    MidiChunk( uint32_t time, uint8_t type, uint8_t data1 )
    {
        _time = time;
        _type = type;
        _binaryTime = packToMIDITime( time );
        _data.push_back( data1 );
    }

    MidiChunk( uint32_t time, uint8_t type, uint8_t data1, uint8_t data2 )
    {
        _time = time;
        _type = type;
        _binaryTime = packToMIDITime( time );
        _data.push_back( data1 );
        _data.push_back( data2 );
    }

    MidiChunk( uint32_t time, uint8_t meta, uint8_t subType, const uint8_t * ptr, uint8_t metaLength )
    {
        _time = time;
        _type = meta;
        _binaryTime = packToMIDITime( time );
        _data.push_back( subType );
        _data.push_back( metaLength );
        for ( uint8_t i = 0; i < metaLength; ++i ) {
            _data.push_back( *( ptr + i ) );
        }
    }

    size_t size( void ) const
    {
        return _binaryTime.size() + 1 + _data.size();
    }
};

static bool operator<( const MidiChunk & left, const MidiChunk & right )
{
    return left._time < right._time;
}

StreamBuf & operator<<( StreamBuf & sb, const MidiChunk & event )
{
    for ( std::vector<u8>::const_iterator it = event._binaryTime.begin(); it != event._binaryTime.end(); ++it )
        sb << *it;
    sb << event._type;
    for ( std::vector<u8>::const_iterator it = event._data.begin(); it != event._data.end(); ++it )
        sb << *it;
    return sb;
}

struct MidiEvents : std::vector<MidiChunk>
{
    uint32_t trackTempo = 0;

    size_t count( void ) const
    {
        return std::vector<MidiChunk>::size();
    }

    size_t size( void ) const
    {
        size_t res = 0;
        for ( const_iterator it = begin(); it != end(); ++it )
            res += ( *it ).size();
        return res;
    }

    MidiEvents() {}
    MidiEvents( const XMITrack & t )
    {
        const u8 * ptr = &t.evnt[0];
        const u8 * end = ptr + t.evnt.size();

        u32 delta = 0;

        while ( ptr && ptr < end ) {
            // XMI delay is 7 bit values summed together
            if ( *ptr < 128 ) {
                delta += *ptr;
                ++ptr;
            }
            else
            // MIDI commands start from 0x80
            {
                // track end
                if ( 0xFF == *ptr && 0x2F == *( ptr + 1 ) ) {
                    push_back( MidiChunk( delta, *ptr, *( ptr + 1 ), *( ptr + 2 ) ) );
                    // stop parsing
                    break;
                }
                else
                    switch ( *ptr >> 4 ) {
                    // metadata
                    case 0x0F: {
                        ptr++; // skip 0xFF
                        const uint8_t metaType = *( ptr++ );
                        const uint8_t metaLength = *( ptr++ );
                        push_back( MidiChunk( delta, 0xFF, metaType, ptr, metaLength ) );
                        // Tempo switch
                        if ( metaType == 0x51 && metaLength == 3 ) {
                            // 24bit big endian
                            trackTempo = ( ( ( *ptr << 8 ) | *( ptr + 1 ) ) << 8 ) | *( ptr + 2 );
                        }
                        ptr += metaLength;
                    } break;

                    // key pressure
                    case 0x0A:
                    // control change
                    case 0x0B:
                    // pitch bend
                    case 0x0E: {
                        push_back( MidiChunk( delta, *ptr, *( ptr + 1 ), *( ptr + 2 ) ) );
                        ptr += 3;
                    } break;

                    // XMI events doesn't have note off events
                    // note on
                    case 0x09: {
                        push_back( MidiChunk( delta, *ptr, *( ptr + 1 ), *( ptr + 2 ) ) );
                        const XMI_Time duration = readXMITime( ptr + 3 );
                        // note off
                        push_back( MidiChunk( delta + duration.first, *ptr - 0x10, *( ptr + 1 ), 0x7F ) );
                        ptr += 3 + duration.second;
                    } break;

                    // program change
                    case 0x0C:
                    // channel aftertouch
                    case 0x0D: {
                        push_back( MidiChunk( delta, *ptr, *( ptr + 1 ) ) );
                        ptr += 2;
                    } break;

                    // unused command
                    default:
                        push_back( MidiChunk( 0, 0xFF, 0x2F, 0 ) );
                        ERROR( "unknown st: 0x" << std::setw( 2 ) << std::setfill( '0' ) << std::hex << static_cast<int>( *ptr )
                                                << ", ln: " << static_cast<int>( &t.evnt[0] + t.evnt.size() - ptr ) );
                        break;
                    }
            }
        }

        std::sort( this->begin(), this->end() );

        // update duration
        delta = 0;
        for ( iterator it = this->begin(); it != this->end(); ++it ) {
            it->_binaryTime = packToMIDITime( it->_time - delta );
            delta = it->_time;
        }
    }
};

StreamBuf & operator<<( StreamBuf & sb, const MidiEvents & st )
{
    for ( MidiEvents::const_iterator it = st.begin(); it != st.end(); ++it ) {
        sb << *it;
    }
    return sb;
}

struct MidTrack
{
    IFFChunkHeader mtrk;
    MidiEvents events;

    MidTrack()
        : mtrk( TAG_MTRK, 0 )
    {}
    MidTrack( const XMITrack & t )
        : mtrk( TAG_MTRK, 0 )
        , events( t )
    {
        mtrk.length = events.size();
    }

    size_t size( void ) const
    {
        return sizeof( mtrk ) + events.size();
    }
};

StreamBuf & operator<<( StreamBuf & sb, const MidTrack & st )
{
    sb << st.mtrk;
    sb << st.events;
    return sb;
}

struct MidTracks : std::list<MidTrack>
{
    size_t count( void ) const
    {
        return std::list<MidTrack>::size();
    }

    size_t size( void ) const
    {
        size_t res = 0;
        for ( const_iterator it = begin(); it != end(); ++it )
            res += ( *it ).size();
        return res;
    }

    MidTracks() {}
    MidTracks( const XMITracks & tracks )
    {
        for ( XMITracks::const_iterator it = tracks.begin(); it != tracks.end(); ++it )
            push_back( MidTrack( *it ) );
    }
};

StreamBuf & operator<<( StreamBuf & sb, const MidTracks & st )
{
    for ( std::list<MidTrack>::const_iterator it = st.begin(); it != st.end(); ++it )
        sb << *it;
    return sb;
}

struct MidData
{
    IFFChunkHeader mthd;
    int format;
    int ppqn;
    MidTracks tracks;

    MidData()
        : mthd( TAG_MTHD, 6 )
        , format( 0 )
        , ppqn( 0 )
    {}
    MidData( const XMITracks & t )
        : mthd( TAG_MTHD, 6 )
        , format( 0 )
        , ppqn( 60 )
        , tracks( t )
    {
        // XMI files play MIDI at a fixed clock rate of 120 Hz
        if ( tracks.size() > 0 && tracks.front().events.trackTempo > 0 ) {
            ppqn = ( tracks.front().events.trackTempo * 3 / 25000 );
        }
    }
};

StreamBuf & operator<<( StreamBuf & sb, const MidData & st )
{
    sb << st.mthd;
    sb.putBE16( st.format );
    sb.putBE16( st.tracks.count() );
    sb.putBE16( st.ppqn );
    sb << st.tracks;
    return sb;
}

std::vector<u8> Music::Xmi2Mid( const std::vector<u8> & buf )
{
    XMIData xmi( buf );
    StreamBuf sb( 16 * 4096 );

    if ( xmi.isvalid() ) {
        MidData mid( xmi.tracks );
        sb << mid;
    }

    return std::vector<u8>( sb.data(), sb.data() + sb.size() );
}
