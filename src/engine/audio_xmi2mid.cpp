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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "audio.h" // IWYU pragma: associated
#include "logging.h"
#include "serialize.h"
#include "tools.h"

// The original MIDI files are stored in XMI format which is not readable by SDL.
// You can read about this format here: https://moddingwiki.shikadi.net/wiki/XMI_Format
//
// A conversion from XMI to MID files is required before playing files.
//
// MIDI format is described here: http://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html
// and here: https://en.wikipedia.org/wiki/General_MIDI

namespace
{
#ifdef WITH_DEBUG
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
#endif

    enum class Tag : uint32_t
    {
        FORM = 0x464F524D,
        XDIR = 0x58444952,
        INFO = 0x494E464F,
        CAT0 = 0x43415420,
        XMID = 0x584D4944,
        TIMB = 0x54494D42,
        EVNT = 0x45564E54,
        RBRN = 0x5242524E,
        MTHD = 0x4D546864,
        MTRK = 0x4D54726B
    };

    // Some numbers in MIDI Files are represented in a form called VARIABLE-LENGTH QUANTITY.
    struct VariableLengthQuantity
    {
        uint32_t value{ 0 };
        uint32_t lengthInBytes{ 0 };
    };

    bool readVariableLengthQuantity( const std::vector<uint8_t>::const_iterator data, const std::vector<uint8_t>::const_iterator dataEnd,
                                     VariableLengthQuantity & quantity )
    {
        quantity = {};

        auto iter = data;

        while ( iter < dataEnd && ( *iter ) > 127 ) {
            if ( ( iter - data ) >= 4 ) {
                // The largest number to read is 4 bytes.
                ERROR_LOG( "XMI parse error: the field size exceeds 4 bytes" )
                return false;
            }

            quantity.value |= 0x0000007F & *iter;
            quantity.value <<= 7;
            ++iter;
        }

        if ( iter < dataEnd ) {
            quantity.value += *iter;
        }

        quantity.lengthInBytes = static_cast<uint32_t>( iter - data ) + 1;

        return true;
    }

    std::vector<uint8_t> packVariableLengthQuantity( const uint32_t delta )
    {
        const uint8_t c1 = static_cast<uint8_t>( delta & 0x0000007F );
        uint8_t c2 = static_cast<uint8_t>( ( delta & 0x00003F80 ) >> 7 );
        uint8_t c3 = static_cast<uint8_t>( ( delta & 0x001FC000 ) >> 14 );
        uint8_t c4 = static_cast<uint8_t>( ( delta & 0x0FE00000 ) >> 21 );

        if ( c4 ) {
            c4 |= 0x80;
            c3 |= 0x80;
            c2 |= 0x80;
            return { c4, c3, c2, c1 };
        }

        if ( c3 ) {
            c3 |= 0x80;
            c2 |= 0x80;
            return { c3, c2, c1 };
        }

        if ( c2 ) {
            c2 |= 0x80;
            return { c2, c1 };
        }

        return { c1 };
    }

    struct IFFChunkHeader
    {
        // ASCII string 4 bytes long, padded with a space character 0x20 if necessary.
        Tag ID{ 0 };
        // Big-endian.
        uint32_t length{ 0 };

        IFFChunkHeader() = default;

        IFFChunkHeader( const Tag id, const uint32_t sz )
            : ID( id )
            , length( sz )
        {
            // Do nothing.
        }

        // Returns the size in bytes that this structure will occupy in serialized binary form inside an XMI or MIDI file
        size_t sizeInBytes() const
        {
            // This header contains of two 4-byte integers.
            return sizeof( ID ) + sizeof( length );
        }
    };

    IStreamBase & operator>>( IStreamBase & stream, IFFChunkHeader & st )
    {
        st.ID = static_cast<Tag>( stream.getBE32() );
        st.length = stream.getBE32();

        return stream;
    }

    OStreamBase & operator<<( OStreamBase & stream, const IFFChunkHeader & st )
    {
        stream.putBE32( static_cast<std::underlying_type_t<Tag>>( st.ID ) );
        stream.putBE32( st.length );

        return stream;
    }

    struct GroupChunkHeader
    {
        // ASCII string 4 bytes long, padded with a space character 0x20 if necessary - either 'FORM', 'CAT ' or 'LIST'.
        Tag ID{ 0 };
        // Big-endian.
        uint32_t length{ 0 };
        // ASCII string 4 bytes long, padded with a space character 0x20 if necessary.
        Tag type{ 0 };
    };

    IStreamBase & operator>>( IStreamBase & stream, GroupChunkHeader & st )
    {
        st.ID = static_cast<Tag>( stream.getBE32() );
        st.length = stream.getBE32();
        st.type = static_cast<Tag>( stream.getBE32() );

        return stream;
    }

    struct VectorSubrange
    {
        std::vector<uint8_t>::const_iterator data;
        const std::vector<uint8_t>::const_iterator dataEnd;
    };

    struct XMIData
    {
        VectorSubrange trackEvents;

        bool isValid{ false };

        explicit XMIData( const std::vector<uint8_t> & buf )
            : trackEvents{ buf.cend(), buf.cend() }
        {
            // Please refer to https://moddingwiki.shikadi.net/wiki/XMI_Format#File_format
            ROStreamBuf sb( buf );

            GroupChunkHeader group;
            sb >> group;

            if ( group.ID != Tag::FORM || group.type != Tag::XDIR ) {
                ERROR_LOG( "XMI parse error: invalid IFF root chunk 1 (FORM:XDIR)" )
                return;
            }

            IFFChunkHeader iff;
            sb >> iff;
            if ( iff.ID != Tag::INFO || iff.length != 2 ) {
                ERROR_LOG( "XMI parse error: invalid TAG_INFO" )
                return;
            }

            if ( const uint16_t numTracks = sb.getLE16(); numTracks != 1 ) {
                ERROR_LOG( "XMI parse error: the number of sequences should be 1 to be properly converted to MID format 0" )
                return;
            }

            // CAT XMID
            sb >> group;
            if ( group.ID != Tag::CAT0 || group.type != Tag::XMID ) {
                ERROR_LOG( "XMI parse error: invalid IFF root chunk 2 (CAT:XMID)" )
                return;
            }

            // FORM XMID
            sb >> group;
            if ( group.ID != Tag::FORM || group.type != Tag::XMID ) {
                ERROR_LOG( "XMI parse error: invalid form type (FORM:XMID)" )
                return;
            }

            sb >> iff;

            // TIMB is not used in MIDI files.
            if ( iff.ID == Tag::TIMB ) {
                sb.skip( iff.length );
                sb >> iff;
            }

            // [RBRN]
            if ( iff.ID == Tag::RBRN ) {
                sb.skip( iff.length );
                sb >> iff;
            }

            // EVNT chunk
            if ( iff.ID != Tag::EVNT ) {
                ERROR_LOG( "XMI parse error: ID is not EVNT" )
                return;
            }

            if ( sb.fail() ) {
                ERROR_LOG( "XMI parse error: I/O error" )
                return;
            }

            // The single track XMI files does not have any data after the EVNT data.
            if ( sb.size() != iff.length ) {
                ERROR_LOG( "XMI parse error: EVNT data is out of range" )
                return;
            }

            // Mark the beginning of the EVNT data.
            trackEvents.data = buf.cbegin() + static_cast<ptrdiff_t>( sb.tell() );
            assert( trackEvents.dataEnd - trackEvents.data == static_cast<ptrdiff_t>( iff.length ) );

            isValid = ( trackEvents.data != trackEvents.dataEnd );
        }
    };

    // codechecker_false_positive [core.uninitialized.Assign] Value assigned to field '_time' in implicit constructor is garbage or undefined
    struct MidiChunk
    {
        uint32_t _time{ 0 };
        uint8_t _type{ 0 };

        // _binaryTime is calculated only after all chunks are pushed and sorted.
        std::vector<uint8_t> _binaryTime;
        std::vector<uint8_t> _data;

        MidiChunk( const uint32_t time, const uint8_t type, const uint8_t data1 )
            : _time( time )
            , _type( type )
            , _data( { data1 } )
        {
            // Do nothing.
        }

        MidiChunk( const uint32_t time, const uint8_t type, const uint8_t data1, const uint8_t data2 )
            : _time( time )
            , _type( type )
            , _data( { data1, data2 } )
        {
            // Do nothing.
        }

        MidiChunk( const uint32_t time, const uint8_t meta, const uint8_t subType, const std::vector<uint8_t>::const_iterator iter, const uint8_t metaLength )
            : _time( time )
            , _type( meta )
        {
            _data.reserve( static_cast<size_t>( 2 ) + metaLength );

            _data.push_back( subType );
            _data.push_back( metaLength );

            for ( uint8_t i = 0; i < metaLength; ++i ) {
                _data.push_back( *( iter + i ) );
            }
        }

        // Returns the size in bytes that this structure will occupy in serialized binary form inside a MIDI file
        size_t sizeInBytes() const
        {
            return _binaryTime.size() + sizeof( _type ) + _data.size();
        }
    };

    bool operator<( const MidiChunk & left, const MidiChunk & right )
    {
        return left._time < right._time;
    }

    OStreamBase & operator<<( OStreamBase & stream, const MidiChunk & event )
    {
        for ( const uint8_t binaryTimeByte : event._binaryTime ) {
            stream << binaryTimeByte;
        }

        stream << event._type;

        for ( const uint8_t dataByte : event._data ) {
            stream << dataByte;
        }

        return stream;
    }

    struct MidiEvents final : public std::vector<MidiChunk>
    {
        uint32_t trackTempo{ 0 };

        MidiEvents() = default;

        explicit MidiEvents( const VectorSubrange & trackEvents )
        {
            assert( trackEvents.data != trackEvents.dataEnd );

            std::vector<uint8_t>::const_iterator iter = trackEvents.data;

            auto checkDataPresence = [this, &trackEvents, &iter]( const int32_t requiredLength ) {
                assert( requiredLength > 0 );
                if ( trackEvents.dataEnd - iter < requiredLength ) {
                    emplace_back( 0, static_cast<uint8_t>( 0xFF ), static_cast<uint8_t>( 0x2F ), static_cast<uint8_t>( 0x00 ) );
                    ERROR_LOG( "MIDI track: the data is truncated" )
                    return false;
                }

                return true;
            };

            uint32_t time = 0;

            while ( iter < trackEvents.dataEnd ) {
                // XMI delay is 7 bit values summed together
                if ( *iter < 128 ) {
                    time += *iter;
                    ++iter;
                    continue;
                }

                if ( *iter == 0xFF ) {
                    if ( !checkDataPresence( 3 ) ) {
                        break;
                    }

                    // Meta-Event, always starts from 0xFF. We skip it.
                    ++iter;

                    if ( *iter == 0x2F ) {
                        if ( *( ++iter ) != 0x00 ) {
                            ERROR_LOG( "MIDI track: End of Track sequence is incorrect" )
                        }

                        emplace_back( time, static_cast<uint8_t>( 0xFF ), static_cast<uint8_t>( 0x2F ), static_cast<uint8_t>( 0x00 ) );
                        break;
                    }

                    const uint8_t metaType = *( iter++ );
                    const uint8_t metaLength = *( iter++ );

                    if ( !checkDataPresence( metaLength ) ) {
                        break;
                    }

                    emplace_back( time, static_cast<uint8_t>( 0xFF ), metaType, iter, metaLength );
                    // Tempo switch
                    if ( metaType == 0x51 && metaLength == 3 ) {
                        // 24-bit big endian
                        trackTempo = ( ( ( *iter << 8 ) | *( iter + 1 ) ) << 8 ) | *( iter + 2 );
                    }

                    iter += metaLength;
                    continue;
                }

                switch ( *iter >> 4 ) {
                // Polyphonic Key Pressure (Aftertouch).
                case 0x0A:
                // Control Change.
                case 0x0B:
                // Pitch Wheel Change.
                case 0x0E:
                    if ( !checkDataPresence( 3 ) ) {
                        break;
                    }

                    emplace_back( time, *iter, *( iter + 1 ), *( iter + 2 ) );
                    iter += 3;
                    break;

                // XMI events do not have note off events.
                // Note On event.
                case 0x09: {
                    if ( !checkDataPresence( 4 ) ) {
                        break;
                    }

                    emplace_back( time, *iter, *( iter + 1 ), *( iter + 2 ) );

                    VariableLengthQuantity quantity;
                    if ( !readVariableLengthQuantity( iter + 3, trackEvents.dataEnd, quantity ) ) {
                        break;
                    }

                    // note off
                    emplace_back( time + quantity.value, static_cast<uint8_t>( *iter - 0x10 ), *( iter + 1 ), static_cast<uint8_t>( 0x7F ) );
                    iter += 3 + quantity.lengthInBytes;
                    break;
                }

                // Program Change: in other words which instrument is going to be played.
                case 0x0C: {
                    if ( !checkDataPresence( 2 ) ) {
                        break;
                    }

                    emplace_back( time, *iter, *( iter + 1 ) );

#ifdef WITH_DEBUG
                    const int32_t channelId = *iter - 0xC0;

                    // Drum sounds are only played in channel 9 if channel ID starts from 0, or 10 if channel ID starts from 1. In our case it starts from 0.
                    if ( channelId == 9 ) {
                        // It is a drum kit.
                        const uint32_t drumKitId = *( iter + 1 );
                        const auto drumKitIter = drumKitDescription.find( drumKitId );

                        if ( drumKitIter != drumKitDescription.end() ) {
                            DEBUG_LOG( DBG_ENGINE, DBG_TRACE, "MIDI channel " << channelId << ", drum kit ID " << drumKitId << ": " << drumKitIter->second )
                        }
                        else {
                            ERROR_LOG( "MIDI channel " << channelId << ": unknown drum kit ID " << drumKitId )
                        }
                    }
                    else {
                        const uint32_t instrumentId = *( iter + 1 );

                        if ( instrumentId < instrumentDescription.size() ) {
                            DEBUG_LOG( DBG_ENGINE, DBG_TRACE,
                                       "MIDI channel " << channelId << ", instrument ID " << instrumentId << ": " << instrumentDescription[instrumentId] )
                        }
                        else {
                            ERROR_LOG( "MIDI channel " << channelId << ": unknown instrument ID " << instrumentId )
                        }
                    }
#endif

                    iter += 2;
                    break;
                }

                // Channel Pressure (After-touch).
                case 0x0D:
                    if ( !checkDataPresence( 2 ) ) {
                        break;
                    }

                    emplace_back( time, *iter, *( iter + 1 ) );
                    iter += 2;
                    break;

                // Unknown command.
                default:
                    emplace_back( 0, static_cast<uint8_t>( 0xFF ), static_cast<uint8_t>( 0x2F ), static_cast<uint8_t>( 0x00 ) );
                    ERROR_LOG( "MIDI track: unknown command: " << GetHexString( static_cast<int>( *iter ), 2 )
                                                               << ", byte: " << static_cast<int>( iter - trackEvents.data ) )
                    break;
                }
            }

            std::stable_sort( begin(), end() );

            // Set binary time according to the sorted chunks order.
            time = 0;
            for ( MidiChunk & chunk : *this ) {
                chunk._binaryTime = packVariableLengthQuantity( chunk._time - time );
                time = chunk._time;
            }
        }

        // Returns the size in bytes that this structure will occupy in serialized binary form inside a MIDI file
        size_t sizeInBytes() const
        {
            return std::accumulate( begin(), end(), static_cast<size_t>( 0 ), []( const size_t total, const MidiChunk & chunk ) { return total + chunk.sizeInBytes(); } );
        }
    };

    OStreamBase & operator<<( OStreamBase & stream, const MidiEvents & st )
    {
        for ( const MidiChunk & chunk : st ) {
            stream << chunk;
        }

        return stream;
    }

    struct MidTrack
    {
        MidiEvents events;
        IFFChunkHeader mtrk{ Tag::MTRK, 0 };

        explicit MidTrack( const VectorSubrange & trackEvents )
            : events( trackEvents )
            , mtrk( Tag::MTRK, static_cast<uint32_t>( events.sizeInBytes() ) )
        {
            // Do nothing.
        }
    };

    OStreamBase & operator<<( OStreamBase & stream, const MidTrack & st )
    {
        stream << st.mtrk;
        stream << st.events;

        return stream;
    }

    struct MidData
    {
        IFFChunkHeader mthd{ Tag::MTHD, 6 };
        uint16_t format{ 0 };
        uint16_t ppqn{ 60 };
        // MIDI format 0 can contain only one track.
        MidTrack track;

        explicit MidData( const VectorSubrange & trackEvents )
            : track( trackEvents )
        {
            // XMI files play MIDI at a fixed clock rate of 120 Hz
            if ( track.events.trackTempo > 0 ) {
                ppqn = static_cast<uint16_t>( track.events.trackTempo * 3 / 25000 );
            }
        }

        // Returns the size in bytes that this structure will occupy in serialized binary form inside a MIDI file
        size_t sizeInBytes() const
        {
            // The total MIDI data size is: MThd header and data length plus MTrk header and data length.
            return mthd.sizeInBytes() + mthd.length + track.mtrk.sizeInBytes() + track.mtrk.length;
        }
    };

    OStreamBase & operator<<( OStreamBase & stream, const MidData & st )
    {
        stream << st.mthd;
        stream.putBE16( st.format );
        // Write that there is one track in midi file.
        stream.putBE16( static_cast<uint16_t>( 1 ) );
        stream.putBE16( st.ppqn );
        stream << st.track;

        return stream;
    }
}

std::vector<uint8_t> Music::Xmi2Mid( const std::vector<uint8_t> & buf )
{
    const XMIData xmi( buf );
    if ( !xmi.isValid ) {
        return {};
    }

    const MidData mid( xmi.trackEvents );

    // Create a buffer for the midi data.
    RWStreamBuf sb( mid.sizeInBytes() );

    sb << mid;

    if ( sb.fail() ) {
        ERROR_LOG( "Error writing MIDI data to the buffer: I/O error" );
        return {};
    }

    return { sb.data(), sb.data() + sb.size() };
}
