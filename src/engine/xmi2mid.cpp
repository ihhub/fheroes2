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

// IWYU pragma: no_include <ext/alloc_traits.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "audio.h"
#include "logging.h"
#include "serialize.h"
#include "tools.h"

// The original MIDI files are stored in XMI format which is not readable by SDL. You can read about this format here: https://moddingwiki.shikadi.net/wiki/XMI_Format
// A conversion from XMI to MID files is required before playing files.
// MIDI format is described here: http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html and here:
// https://en.wikipedia.org/wiki/General_MIDI

namespace
{
    const std::array<const char *, 128> instrumentDescription = {
        //
        // Piano ( 0 - 7 )
        //
        "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano", "Electric Piano 1 (Rhodes Piano)",
        "Electric Piano 2 (Chorused Piano)", "Harpsichord", "Clavinet",
        //
        // Chromatic Percussion ( 8 - 15 )
        //
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer (Santur)",
        //
        // Organ ( 16 - 23 )
        //
        "Drawbar Organ (Hammond)", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion (French)", "Harmonica", "Tango Accordion (Band neon)",
        //
        // Guitar ( 24 - 31 )
        //
        "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)", "Electric Guitar (muted)", "Overdriven Guitar",
        "Distortion Guitar", "Guitar harmonics",
        //
        // Bass ( 32 - 39 )
        //
        "Acoustic Bass", "Electric Bass (fingered)", "Electric Bass (picked)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        //
        // Strings ( 40 - 47 )
        //
        "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
        //
        // Strings ( 48 - 55 )
        //
        "String Ensemble 1 (strings)", "String Ensemble 2 (slow strings)", "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
        //
        // Brass ( 56 - 63 )
        //
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2",
        //
        // Reed ( 64 - 71 )
        //
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
        //
        // Pipe ( 72 - 79 )
        //
        "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
        //
        // Synth Lead ( 80 - 87 )
        //
        "Lead 1 (square wave)", "Lead 2 (sawtooth wave)", "Lead 3 (calliope)", "Lead 4 (chiffer)", "Lead 5 (charang)", "Lead 6 (voice solo)", "Lead 7 (fifths)",
        "Lead 8 (bass + lead)",
        //
        // Synth Pad ( 88 - 95 )
        //
        "Pad 1 (new age Fantasia)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir space voice)", "Pad 5 (bowed glass)", "Pad 6 (metallic pro)", "Pad 7 (halo)",
        "Pad 8 (sweep)",
        //
        // Synth Effects ( 96 - 103 )
        //
        "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)", "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes, drops)",
        "FX 8 (sci-fi, star theme)",
        //
        // Ethnic ( 104 - 111 )
        //
        "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bag pipe", "Fiddle", "Shanai",
        //
        // Percussive ( 112 - 119 )
        //
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
        //
        // Sound effects ( 120 - 127 )
        //
        "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot" };

    // Some (but not all) commonly used MIDI drum kits are listed here: https://en.wikipedia.org/wiki/General_MIDI_Level_2#Drum_sounds
    const std::map<uint32_t, const char *> drumKitDescription
        = { { 0, "Standard Kit" }, { 8, "Room Kit" },   { 16, "Power Kit" },      { 24, "Electronic Kit" }, { 25, "TR-808 Kit" },
            { 32, "Jazz Kit" },    { 40, "Brush Kit" }, { 48, "Orchestral Kit" }, { 49, "Fix Room Kit" },   { 56, "Sound FX Kit" } };

    enum
    {
        TAG_FORM = 0x464F524D,
        TAG_XDIR = 0x58444952,
        TAG_INFO = 0x494E464F,
        TAG_CAT0 = 0x43415420,
        TAG_XMID = 0x584D4944,
        TAG_TIMB = 0x54494D42,
        TAG_EVNT = 0x45564E54,
        TAG_RBRN = 0x5242524E,
        TAG_MTHD = 0x4D546864,
        TAG_MTRK = 0x4D54726B
    };

    // Some numbers in MIDI Files are represented in a form called VARIABLE-LENGTH QUANTITY.
    struct VariableLengthQuantity
    {
        uint32_t value{ 0 };
        uint32_t lengthInBytes{ 0 };
    };

    bool readVariableLengthQuantity( const uint8_t * data, const uint8_t * dataEnd, VariableLengthQuantity & quantity )
    {
        quantity = {};

        const uint8_t * p = data;

        while ( p < dataEnd && ( *p & 0x80 ) != 0 ) {
            if ( 4 <= p - data ) {
                // The largest number to read is 4 bytes.
                ERROR_LOG( "XMI format: the field is bigger than 4 bytes." )
                return false;
            }

            quantity.value |= 0x0000007F & *p;
            quantity.value <<= 7;
            ++p;
        }

        if ( p < dataEnd ) {
            quantity.value += *p;
        }

        quantity.lengthInBytes = static_cast<uint32_t>( p - data ) + 1; // it's safe to cast since p is always bigger or equal to data

        return true;
    }

    std::vector<uint8_t> packVariableLengthQuantity( const uint32_t delta )
    {
        const uint8_t c1 = delta & 0x0000007F;
        const uint8_t c2 = ( ( delta & 0x00003F80 ) >> 7 ) & 0xFF;
        const uint8_t c3 = ( ( delta & 0x001FC000 ) >> 14 ) & 0xFF;
        const uint8_t c4 = ( ( delta & 0x0FE00000 ) >> 21 ) & 0xFF;

        std::vector<uint8_t> res;
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
        else {
            res.push_back( c1 );
        }

        return res;
    }
}

struct IFFChunkHeader
{
    uint32_t ID; // 4 upper case ASCII chars, padded with 0x20 (space)
    uint32_t length; // big-endian

    IFFChunkHeader( uint32_t id, uint32_t sz )
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
    uint32_t ID{ 0 }; // 4 byte ASCII string, either 'FORM', 'CAT ' or 'LIST'
    uint32_t length{ 0 };
    uint32_t type{ 0 }; // 4 byte ASCII string
};

StreamBuf & operator>>( StreamBuf & sb, GroupChunkHeader & st )
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    st.type = sb.getBE32();
    return sb;
}

struct XMITrack
{
    std::vector<uint8_t> timb;
    std::vector<uint8_t> evnt;
};

using XMITracks = std::list<XMITrack>;

struct XMIData
{
    XMITracks tracks;

    explicit XMIData( const std::vector<uint8_t> & buf )
    {
        // Please refer to https://moddingwiki.shikadi.net/wiki/XMI_Format#File_format
        StreamBuf sb( buf );

        GroupChunkHeader group;
        sb >> group;

        if ( group.ID != TAG_FORM || group.type != TAG_XDIR ) {
            ERROR_LOG( "XMI parsing: invalid IFF root chunk 1 (FORM:XDIR)" )
            return;
        }

        IFFChunkHeader iff;
        sb >> iff;
        if ( iff.ID != TAG_INFO || iff.length != 2 ) {
            ERROR_LOG( "XMI parsing: expected TAG_INFO of length 2" )
            return;
        }

        const int numTracks = sb.getLE16();
        if ( numTracks <= 0 ) {
            ERROR_LOG( "XMI parsing: the number of sequences cannot be less than 1" )
            return;
        }

        // CAT XMID
        sb >> group;
        if ( group.ID != TAG_CAT0 || group.type != TAG_XMID ) {
            ERROR_LOG( "XMI parsing: invalid IFF root chunk 2 (CAT :XMID)" )
            return;
        }

        for ( int track = 0; track < numTracks; ++track ) {
            tracks.emplace_back();

            std::vector<uint8_t> & timb = tracks.back().timb;
            std::vector<uint8_t> & evnt = tracks.back().evnt;

            sb >> group;
            // FORM XMID
            if ( group.ID != TAG_FORM || group.type != TAG_XMID ) {
                ERROR_LOG( "XMI parsing: invalid form type (FORM:XMID)" )
                return;
            }

            sb >> iff;

            // Read TIMB cbhunk.
            if ( iff.ID == TAG_TIMB ) {
                timb = sb.getRaw( iff.length );
                if ( timb.size() != iff.length ) {
                    ERROR_LOG( "parse error: "
                               << "out of range" )
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
                ERROR_LOG( "parse error: "
                           << "evnt" )
                break;
            }

            evnt = sb.getRaw( iff.length );

            if ( evnt.size() != iff.length ) {
                ERROR_LOG( "parse error: "
                           << "out of range" )
                break;
            }
        }
    }

    bool isvalid() const
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
        _binaryTime = packVariableLengthQuantity( time );
        _data.push_back( data1 );
    }

    MidiChunk( uint32_t time, uint8_t type, uint8_t data1, uint8_t data2 )
    {
        _time = time;
        _type = type;
        _binaryTime = packVariableLengthQuantity( time );
        _data.push_back( data1 );
        _data.push_back( data2 );
    }

    MidiChunk( uint32_t time, uint8_t meta, uint8_t subType, const uint8_t * ptr, uint8_t metaLength )
    {
        _time = time;
        _type = meta;
        _binaryTime = packVariableLengthQuantity( time );
        _data.push_back( subType );
        _data.push_back( metaLength );
        for ( uint8_t i = 0; i < metaLength; ++i ) {
            _data.push_back( *( ptr + i ) );
        }
    }

    size_t size() const
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
    for ( std::vector<uint8_t>::const_iterator it = event._binaryTime.begin(); it != event._binaryTime.end(); ++it )
        sb << *it;

    sb << event._type;

    for ( std::vector<uint8_t>::const_iterator it = event._data.begin(); it != event._data.end(); ++it )
        sb << *it;

    return sb;
}

struct MidiEvents : public std::vector<MidiChunk>
{
    uint32_t trackTempo = 0;

    size_t size() const
    {
        size_t res = 0;
        for ( const_iterator it = begin(); it != end(); ++it )
            res += ( *it ).size();
        return res;
    }

    MidiEvents() = default;

    bool checkDataPresence( const uint8_t * ptr, const uint8_t * end, const int32_t requiredLength )
    {
        assert( requiredLength > 0 );
        if ( end - ptr < requiredLength ) {
            emplace_back( 0, static_cast<uint8_t>( 0xFF ), static_cast<uint8_t>( 0x2F ), static_cast<uint8_t>( 0x00 ) );
            ERROR_LOG( "MIDI track: the data is truncated." )
            return false;
        }

        return true;
    }

    explicit MidiEvents( const XMITrack & t )
    {
        const uint8_t * ptr = &t.evnt[0];
        const uint8_t * end = ptr + t.evnt.size();

        uint32_t delta = 0;

        while ( ptr && ptr < end ) {
            // XMI delay is 7 bit values summed together
            if ( *ptr < 128 ) {
                delta += *ptr;
                ++ptr;
                continue;
            }

            if ( *ptr == 0xFF ) {
                if ( !checkDataPresence( ptr, end, 3 ) ) {
                    break;
                }

                if ( *( ptr + 1 ) == 0x2F ) {
                    if ( *( ptr + 2 ) != 0x00 ) {
                        ERROR_LOG( "MIDI track: End of Track sequence is incorrect." )
                    }

                    emplace_back( delta, *ptr, *( ptr + 1 ), static_cast<uint8_t>( 0x00 ) );
                    break;
                }

                // Meta-Event, always starts from 0xFF.
                ++ptr; // skip 0xFF
                const uint8_t metaType = *( ptr++ );
                const uint8_t metaLength = *( ptr++ );

                if ( !checkDataPresence( ptr, end, metaLength ) ) {
                    break;
                }

                emplace_back( delta, static_cast<uint8_t>( 0xFF ), metaType, ptr, metaLength );
                // Tempo switch
                if ( metaType == 0x51 && metaLength == 3 ) {
                    // 24-bit big endian
                    trackTempo = ( ( ( *ptr << 8 ) | *( ptr + 1 ) ) << 8 ) | *( ptr + 2 );
                }

                ptr += metaLength;
                continue;
            }

            switch ( *ptr >> 4 ) {
            // Polyphonic Key Pressure (Aftertouch).
            case 0x0A:
            // Control Change.
            case 0x0B:
            // Pitch Wheel Change.
            case 0x0E:
                if ( !checkDataPresence( ptr, end, 3 ) ) {
                    break;
                }

                emplace_back( delta, *ptr, *( ptr + 1 ), *( ptr + 2 ) );
                ptr += 3;
                break;

            // XMI events do not have note off events.
            // Note On event.
            case 0x09: {
                if ( !checkDataPresence( ptr, end, 4 ) ) {
                    break;
                }

                emplace_back( delta, *ptr, *( ptr + 1 ), *( ptr + 2 ) );

                VariableLengthQuantity quantity;
                if ( !readVariableLengthQuantity( ptr + 3, end, quantity ) ) {
                    break;
                }

                // note off
                emplace_back( delta + quantity.value, static_cast<uint8_t>( *ptr - 0x10 ), *( ptr + 1 ), static_cast<uint8_t>( 0x7F ) );
                ptr += 3 + quantity.lengthInBytes;
                break;
            }

            // Program Change: in other words which instrument is going to be played.
            case 0x0C: {
                if ( !checkDataPresence( ptr, end, 2 ) ) {
                    break;
                }

                const int32_t channelId = *ptr - 0xC0;

                emplace_back( delta, *ptr, *( ptr + 1 ) );

                // Drum sounds are only played in channel 9 if channel ID starts from 0, or 10 if channel ID starts from 1. In our case it starts from 0.
                if ( channelId == 9 ) {
                    // It is a drum kit.
                    const uint32_t drumKitId = *( ptr + 1 );
                    const auto drumKitIter = drumKitDescription.find( drumKitId );

                    if ( drumKitIter != drumKitDescription.end() ) {
                        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "MIDI channel " << channelId << ", drum kit ID " << drumKitId << ": " << drumKitIter->second )
                    }
                    else {
                        DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "MIDI channel " << channelId << ": unknown drum kit ID " << drumKitId )
                    }
                }
                else {
                    const uint32_t instrumentId = *( ptr + 1 );

                    if ( instrumentId < instrumentDescription.size() ) {
                        DEBUG_LOG( DBG_ENGINE, DBG_TRACE,
                                   "MIDI channel " << channelId << ", instrument ID " << instrumentId << ": " << instrumentDescription[instrumentId] )
                    }
                    else {
                        ERROR_LOG( "MIDI channel " << channelId << ": unknown instrument ID " << instrumentId )
                    }
                }

                ptr += 2;
                break;
            }

            // Channel Pressure (After-touch).
            case 0x0D:
                if ( !checkDataPresence( ptr, end, 2 ) ) {
                    break;
                }

                emplace_back( delta, *ptr, *( ptr + 1 ) );
                ptr += 2;
                break;

            // Unknown command.
            default:
                emplace_back( 0, static_cast<uint8_t>( 0xFF ), static_cast<uint8_t>( 0x2F ), static_cast<uint8_t>( 0x00 ) );
                ERROR_LOG( "MIDI track: Unknown command: " << GetHexString( static_cast<int>( *ptr ), 2 )
                                                           << ", byte: " << static_cast<int>( &t.evnt[0] + t.evnt.size() - ptr ) )
                break;
            }
        }

        std::stable_sort( this->begin(), this->end() );

        // update duration
        delta = 0;
        for ( iterator it = this->begin(); it != this->end(); ++it ) {
            it->_binaryTime = packVariableLengthQuantity( it->_time - delta );
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

    explicit MidTrack( const XMITrack & t )
        : mtrk( TAG_MTRK, 0 )
        , events( t )
    {
        mtrk.length = static_cast<uint32_t>( events.size() );
    }

    size_t size() const
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
    size_t count() const
    {
        return std::list<MidTrack>::size();
    }

    MidTracks() = default;

    explicit MidTracks( const XMITracks & tracks )
    {
        for ( XMITracks::const_iterator it = tracks.begin(); it != tracks.end(); ++it )
            emplace_back( *it );
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

    explicit MidData( const XMITracks & t )
        : mthd( TAG_MTHD, 6 )
        , format( 0 )
        , ppqn( 60 )
        , tracks( t )
    {
        // MIDI format 0 can contain only one track
        assert( tracks.count() == 1 );

        // XMI files play MIDI at a fixed clock rate of 120 Hz
        if ( !tracks.empty() && tracks.front().events.trackTempo > 0 ) {
            ppqn = ( tracks.front().events.trackTempo * 3 / 25000 );
        }
    }
};

StreamBuf & operator<<( StreamBuf & sb, const MidData & st )
{
    sb << st.mthd;
    sb.putBE16( static_cast<uint16_t>( st.format ) );
    sb.putBE16( static_cast<uint16_t>( st.tracks.count() ) );
    sb.putBE16( static_cast<uint16_t>( st.ppqn ) );
    sb << st.tracks;
    return sb;
}

std::vector<uint8_t> Music::Xmi2Mid( const std::vector<uint8_t> & buf )
{
    XMIData xmi( buf );
    StreamBuf sb( 16 * 4096 );

    if ( xmi.isvalid() ) {
        MidData mid( xmi.tracks );
        sb << mid;
    }

    return std::vector<uint8_t>( sb.data(), sb.data() + sb.size() );
}
