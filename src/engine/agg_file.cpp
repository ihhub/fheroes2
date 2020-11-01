#include <iterator>
#include <string>

#include "agg_file.h"

namespace fheroes2
{
    AGGFile::AGGFile() {}

    uint32_t AGGFile::aggFilenameHash( const std::string & s )
    {
        uint32_t a = 0, b = 0;
        for ( auto cri = s.crbegin(); cri != s.crend(); cri++ ) {
            a = ( a << 5 ) + ( a >> 25 );
            uint8_t c = static_cast<uint8_t>( std::toupper( *cri ) );
            b += c;
            a += b + c;
        }
        return a;
    }

    bool AGGFile::isGood() const
    {
        return !stream.fail() && files.size();
    }

    bool AGGFile::Open( const std::string & filename )
    {
        if ( !stream.open( filename, "rb" ) )
            return false;

        const size_t size = stream.size();
        const size_t count = stream.getLE16();
        if ( count * ( sizeof( uint32_t ) * 3 + maxFilenameSize ) >= size )
            return false;
        StreamBuf fileEntries = stream.toStreamBuf( count * sizeof( uint32_t ) * 3 );
        stream.seek( size - maxFilenameSize * count );
        StreamBuf nameEntries = stream.toStreamBuf( maxFilenameSize * count );

        for ( uint16_t i = 0; i < count; ++i ) {
            const uint32_t hash = fileEntries.getLE32();
            const std::string & name = nameEntries.toString( maxFilenameSize );
            if ( hash == aggFilenameHash( name ) && !files.count( hash ) ) {
                auto s1 = fileEntries.getLE32();
                auto s2 = fileEntries.getLE32();
                files[hash] = std::make_pair( s2, s1 );
                names[name] = hash;
            }
            else {
                files.clear();
                names.clear();
                return false;
            }
        }
        return !stream.fail();
    }

    const std::vector<uint8_t> & AGGFile::Read( uint32_t hash )
    {
        auto it = files.find( hash );
        if ( it != files.end() ) {
            auto f = it->second;
            if ( f.first ) {
                stream.seek( f.second );
                body = stream.getRaw( f.first );
                return body;
            }
        }
        body.clear();
        key.clear();
        return body;
    }

    const std::vector<uint8_t> & AGGFile::Read( const std::string & str )
    {
        if ( key != str ) {
            auto it = names.find( str );
            if ( it != names.end() ) {
                key = str;
                return Read( it->second );
            }
        }
        body.clear();
        key.clear();
        return body;
    }
}
