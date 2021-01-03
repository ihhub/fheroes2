/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "agg.h"
#include "artifact.h"
#include "audio_music.h"
#include "dir.h"
#include "engine.h"
#include "error.h"
#include "font.h"
#include "game.h"
#include "m82.h"
#include "mus.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "xmi.h"

#ifdef WITH_ZLIB
#include "embedded_image.h"
#include "zzlib.h"
#endif

#define FATSIZENAME 15

namespace AGG
{
    class FAT
    {
    public:
        FAT()
            : crc( 0 )
            , offset( 0 )
            , size( 0 )
        {}

        u32 crc;
        u32 offset;
        u32 size;

        std::string Info( void ) const;
    };

    class File
    {
    public:
        File();
        ~File();

        bool Open( const std::string & );
        bool isGood( void ) const;
        const std::string & Name( void ) const;
        const FAT & Fat( const std::string & key );

        const std::vector<u8> & Read( const std::string & key );

    private:
        std::string filename;
        std::map<std::string, FAT> fat;
        u32 count_items;
        StreamFile stream;
        std::string key;
        std::vector<u8> body;
    };

    // struct fnt_cache_t
    // {
    //     Surface sfs[4]; /* small_white, small_yellow, medium_white, medium_yellow */
    // };

    struct loop_sound_t
    {
        loop_sound_t( int w, int c )
            : sound( w )
            , channel( c )
        {}

        bool operator==( int m82 ) const
        {
            return m82 == sound;
        }

        int sound;
        int channel;
    };

    File heroes2_agg;
    File heroes2x_agg;

    std::map<int, std::vector<u8> > wav_cache;
    std::map<int, std::vector<u8> > mid_cache;
    std::vector<loop_sound_t> loop_sounds;
    // std::map<u32, fnt_cache_t> fnt_cache;

#ifdef WITH_TTF
    FontTTF * fonts; /* small, medium */

    // void LoadTTFChar( u32 );
#endif

    const std::vector<u8> & GetWAV( int m82 );
    const std::vector<u8> & GetMID( int xmi );

    void LoadWAV( int m82, std::vector<u8> & );
    void LoadMID( int xmi, std::vector<u8> & );

    void LoadFNT( void );

    bool ReadDataDir( void );
    const std::vector<u8> & ReadChunk( const std::string & key, bool ignoreExpansion = false );

    /* return letter sprite */
    // Surface GetUnicodeLetter( u32 ch, u32 ft )
    // {
    //     bool ttf_valid = fonts[0].isValid() && fonts[1].isValid();
    //
    //     if ( !ttf_valid )
    //         return Surface();
    //
    //     if ( !fnt_cache[ch].sfs[0].isValid() )
    //         LoadTTFChar( ch );
    //
    //     switch ( ft ) {
    //     case Font::YELLOW_SMALL:
    //         return fnt_cache[ch].sfs[1];
    //     case Font::BIG:
    //         return fnt_cache[ch].sfs[2];
    //     case Font::YELLOW_BIG:
    //         return fnt_cache[ch].sfs[3];
    //     default:
    //         break;
    //     }
    //
    //     return fnt_cache[ch].sfs[0];
    // }
}

/*AGG::File constructor */
AGG::File::File( void )
    : count_items( 0 )
{}

bool AGG::File::Open( const std::string & fname )
{
    filename = fname;

    if ( !stream.open( filename, "rb" ) ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "error read file: " << filename << ", skipping..." );
        return false;
    }

    const size_t size = stream.size();
    count_items = stream.getLE16();
    DEBUG( DBG_ENGINE, DBG_INFO, "load: " << filename << ", count items: " << count_items );

    StreamBuf fats = stream.toStreamBuf( count_items * 4 * 3 /* crc, offset, size */ );
    stream.seek( size - FATSIZENAME * count_items );
    StreamBuf names = stream.toStreamBuf( FATSIZENAME * count_items );

    for ( u32 ii = 0; ii < count_items; ++ii ) {
        FAT & f = fat[names.toString( FATSIZENAME )];

        f.crc = fats.getLE32();
        f.offset = fats.getLE32();
        f.size = fats.getLE32();
    }

    return !stream.fail();
}

AGG::File::~File() {}

bool AGG::File::isGood( void ) const
{
    return !stream.fail() && count_items;
}

/* get AGG file name */
const std::string & AGG::File::Name( void ) const
{
    return filename;
}

/* get FAT element */
const AGG::FAT & AGG::File::Fat( const std::string & key_ )
{
    return fat[key_];
}

/* dump FAT */
std::string AGG::FAT::Info( void ) const
{
    std::ostringstream os;

    os << "crc: " << crc << ", offset: " << offset << ", size: " << size;
    return os.str();
}

/* read element to body */
const std::vector<u8> & AGG::File::Read( const std::string & str )
{
    if ( key != str ) {
        std::map<std::string, FAT>::const_iterator it = fat.find( str );

        if ( it != fat.end() ) {
            const FAT & f = ( *it ).second;
            key = str;

            if ( f.size ) {
                DEBUG( DBG_ENGINE, DBG_TRACE, key << ":\t" << f.Info() );

                stream.seek( f.offset );
                body = stream.getRaw( f.size );
            }
        }
        else if ( body.size() ) {
            body.clear();
            key.clear();
        }
    }

    return body;
}

/* read data directory */
bool AGG::ReadDataDir( void )
{
    Settings & conf = Settings::Get();

    ListFiles aggs = conf.GetListFiles( "data", ".agg" );
    const std::string & other_data = conf.GetDataParams();

    if ( other_data.size() && other_data != "data" )
        aggs.Append( conf.GetListFiles( other_data, ".agg" ) );

    // not found agg, exit
    if ( 0 == aggs.size() )
        return false;

    // attach agg files
    for ( ListFiles::const_iterator it = aggs.begin(); it != aggs.end(); ++it ) {
        std::string lower = StringLower( *it );
        if ( std::string::npos != lower.find( "heroes2.agg" ) && !heroes2_agg.isGood() )
            heroes2_agg.Open( *it );
        if ( std::string::npos != lower.find( "heroes2x.agg" ) && !heroes2x_agg.isGood() )
            heroes2x_agg.Open( *it );
    }

    conf.SetPriceLoyaltyVersion( heroes2x_agg.isGood() );

    return heroes2_agg.isGood();
}

const std::vector<u8> & AGG::ReadChunk( const std::string & key, bool ignoreExpansion )
{
    if ( !ignoreExpansion && heroes2x_agg.isGood() ) {
        const std::vector<u8> & buf = heroes2x_agg.Read( key );
        if ( buf.size() )
            return buf;
    }

    return heroes2_agg.Read( key );
}

struct ICNHeader
{
    ICNHeader()
        : offsetX( 0 )
        , offsetY( 0 )
        , width( 0 )
        , height( 0 )
        , animationFrames( 0 )
        , offsetData( 0 )
    {}

    u16 offsetX;
    u16 offsetY;
    u16 width;
    u16 height;
    u8 animationFrames; // used for adventure map animations, this can replace ICN::AnimationFrame
    u32 offsetData;
};

StreamBuf & operator>>( StreamBuf & st, ICNHeader & icn )
{
    icn.offsetX = st.getLE16();
    icn.offsetY = st.getLE16();
    icn.width = st.getLE16();
    icn.height = st.getLE16();
    icn.animationFrames = st.get();
    icn.offsetData = st.getLE32();

    return st;
}

/* load 82M object to AGG::Cache in Audio::CVT */
void AGG::LoadWAV( int m82, std::vector<u8> & v )
{
#ifdef WITH_MIXER
    const Settings & conf = Settings::Get();

    if ( conf.UseAltResource() ) {
        std::string name = StringLower( M82::GetString( m82 ) );
        std::string prefix_sounds = System::ConcatePath( "files", "sounds" );

        // ogg
        StringReplace( name, ".82m", ".ogg" );
        std::string sound = Settings::GetLastFile( prefix_sounds, name );
        v = LoadFileToMem( sound );

        if ( v.empty() ) {
            // find mp3
            StringReplace( name, ".82m", ".mp3" );
            sound = Settings::GetLastFile( prefix_sounds, name );

            v = LoadFileToMem( sound );
        }

        if ( v.size() ) {
            DEBUG( DBG_ENGINE, DBG_INFO, sound );
            return;
        }
    }
#endif

    DEBUG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
    const std::vector<u8> & body = ReadChunk( M82::GetString( m82 ) );

    if ( body.size() ) {
#ifdef WITH_MIXER
        // create WAV format
        StreamBuf wavHeader( 44 );
        wavHeader.putLE32( 0x46464952 ); // RIFF
        wavHeader.putLE32( body.size() + 0x24 ); // size
        wavHeader.putLE32( 0x45564157 ); // WAVE
        wavHeader.putLE32( 0x20746D66 ); // FMT
        wavHeader.putLE32( 0x10 ); // size_t
        wavHeader.putLE16( 0x01 ); // format
        wavHeader.putLE16( 0x01 ); // channels
        wavHeader.putLE32( 22050 ); // samples
        wavHeader.putLE32( 22050 ); // byteper
        wavHeader.putLE16( 0x01 ); // align
        wavHeader.putLE16( 0x08 ); // bitsper
        wavHeader.putLE32( 0x61746164 ); // DATA
        wavHeader.putLE32( static_cast<uint32_t>( body.size() ) ); // size

        v.reserve( body.size() + 44 );
        v.assign( wavHeader.data(), wavHeader.data() + 44 );
        v.insert( v.begin() + 44, body.begin(), body.end() );
#else
        Audio::Spec wav_spec;
        wav_spec.format = AUDIO_U8;
        wav_spec.channels = 1;
        wav_spec.freq = 22050;

        const Audio::Spec & hardware = Audio::GetHardwareSpec();

        Audio::CVT cvt;

        if ( cvt.Build( wav_spec, hardware ) ) {
            const u32 size = cvt.len_mult * body.size();

            cvt.buf = new u8[size];
            cvt.len = body.size();

            memcpy( cvt.buf, &body[0], body.size() );

            cvt.Convert();

            v.assign( cvt.buf, cvt.buf + size - 1 );

            delete[] cvt.buf;
            cvt.buf = NULL;
        }
#endif
    }
}

/* load XMI object */
void AGG::LoadMID( int xmi, std::vector<u8> & v )
{
    DEBUG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) );
    const std::vector<u8> & body = ReadChunk( XMI::GetString( xmi ), xmi >= XMI::MIDI_ORIGINAL_KNIGHT );

    if ( body.size() )
        v = Music::Xmi2Mid( body );
}

/* return CVT */
const std::vector<u8> & AGG::GetWAV( int m82 )
{
    std::vector<u8> & v = wav_cache[m82];
    if ( Mixer::isValid() && v.empty() )
        LoadWAV( m82, v );
    return v;
}

/* return MID */
const std::vector<u8> & AGG::GetMID( int xmi )
{
    std::vector<u8> & v = mid_cache[xmi];
    if ( Mixer::isValid() && v.empty() )
        LoadMID( xmi, v );
    return v;
}

void AGG::LoadLOOPXXSounds( const std::vector<int> & vols )
{
    const Settings & conf = Settings::Get();

    if ( conf.Sound() ) {
        // set volume loop sounds
        for ( std::vector<int>::const_iterator itv = vols.begin(); itv != vols.end(); ++itv ) {
            int vol = *itv;
            int m82 = M82::GetLOOP00XX( std::distance( vols.begin(), itv ) );
            if ( M82::UNKNOWN == m82 )
                continue;

            // find loops
            std::vector<loop_sound_t>::iterator itl = std::find( loop_sounds.begin(), loop_sounds.end(), m82 );

            if ( itl != loop_sounds.end() ) {
                // unused and free
                if ( 0 == vol ) {
                    if ( Mixer::isPlaying( ( *itl ).channel ) ) {
                        Mixer::Pause( ( *itl ).channel );
                        Mixer::Volume( ( *itl ).channel, Mixer::MaxVolume() * conf.SoundVolume() / 10 );
                        Mixer::Stop( ( *itl ).channel );
                    }
                    ( *itl ).sound = M82::UNKNOWN;
                }
                // used and set vols
                else if ( Mixer::isPlaying( ( *itl ).channel ) ) {
                    Mixer::Pause( ( *itl ).channel );
                    Mixer::Volume( ( *itl ).channel, vol * conf.SoundVolume() / 10 );
                    Mixer::Resume( ( *itl ).channel );
                }
            }
            else
                // new sound
                if ( 0 != vol ) {
                const std::vector<u8> & v = GetWAV( m82 );
                const int ch = Mixer::Play( &v[0], v.size(), -1, true );

                if ( 0 <= ch ) {
                    Mixer::Pause( ch );
                    Mixer::Volume( ch, vol * conf.SoundVolume() / 10 );
                    Mixer::Resume( ch );

                    // find unused
                    itl = std::find( loop_sounds.begin(), loop_sounds.end(), static_cast<int>( M82::UNKNOWN ) );

                    if ( itl != loop_sounds.end() ) {
                        ( *itl ).sound = m82;
                        ( *itl ).channel = ch;
                    }
                    else
                        loop_sounds.push_back( loop_sound_t( m82, ch ) );

                    DEBUG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
                }
            }
        }
    }
}

/* wrapper Audio::Play */
void AGG::PlaySound( int m82 )
{
    const Settings & conf = Settings::Get();

    if ( conf.Sound() ) {
        DEBUG( DBG_ENGINE, DBG_TRACE, M82::GetString( m82 ) );
        const std::vector<u8> & v = AGG::GetWAV( m82 );
        const int ch = Mixer::Play( &v[0], v.size(), -1, false );
        Mixer::Pause( ch );
        Mixer::Volume( ch, Mixer::MaxVolume() * conf.SoundVolume() / 10 );
        Mixer::Resume( ch );
    }
}

/* wrapper Audio::Play */
void AGG::PlayMusic( int mus, bool loop )
{
    const Settings & conf = Settings::Get();

    if ( !conf.Music() || MUS::UNUSED == mus || MUS::UNKNOWN == mus || ( Game::CurrentMusic() == mus && Music::isPlaying() ) )
        return;

    Game::SetCurrentMusic( mus );
    const std::string prefix_music( "music" );
    const MusicSource type = conf.MusicType();

    bool isSongFound = false;

    if ( type == MUSIC_EXTERNAL ) {
        std::string filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::DOS_VERSION ) );

        if ( !System::IsFile( filename ) ) {
            filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::WIN_VERSION ) );
            if ( !System::IsFile( filename ) ) {
                filename.clear();
            }
        }

        if ( filename.empty() ) {
            filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::MAPPED ) );

            if ( !System::IsFile( filename ) ) {
                StringReplace( filename, ".ogg", ".mp3" );

                if ( !System::IsFile( filename ) ) {
                    DEBUG( DBG_ENGINE, DBG_WARN, "error read file: " << Settings::GetLastFile( prefix_music, MUS::GetString( mus, MUS::MAPPED ) ) << ", skipping..." );
                    filename.clear();
                }
            }
        }

        if ( filename.size() ) {
            Music::Play( filename, loop );
            isSongFound = true;
        }
        DEBUG( DBG_ENGINE, DBG_TRACE, MUS::GetString( mus, MUS::MAPPED ) );
    }
#ifdef WITH_AUDIOCD
    else if ( type == MUSIC_CDROM && Cdrom::isValid() ) {
        Cdrom::Play( mus, loop );
        isSongFound = true;
        DEBUG( DBG_ENGINE, DBG_INFO, "cd track " << static_cast<int>( mus ) );
    }
#endif

    if ( !isSongFound ) {
        // Check if music needs to be pulled from HEROES2X
        int xmi = XMI::UNKNOWN;
        if ( type == MUSIC_MIDI_EXPANSION ) {
            xmi = XMI::FromMUS( mus, heroes2x_agg.isGood() );
        }

        if ( XMI::UNKNOWN == xmi ) {
            xmi = XMI::FromMUS( mus, false );
        }

        if ( XMI::UNKNOWN != xmi ) {
#ifdef WITH_MIXER
            const std::vector<u8> & v = GetMID( xmi );
            if ( v.size() )
                Music::Play( v, loop );
#else
            std::string mid = XMI::GetString( xmi );
            StringReplace( mid, ".XMI", ".MID" );
            const std::string file = System::ConcatePath( Settings::GetWriteableDir( "music" ), mid );

            if ( !System::IsFile( file ) )
                SaveMemToFile( GetMID( xmi ), file );

            Music::Play( file, loop );
#endif
        }
        DEBUG( DBG_ENGINE, DBG_TRACE, XMI::GetString( xmi ) );
    }
}

#ifdef WITH_TTF
// void AGG::LoadTTFChar( u32 ch )
// {
//     const Settings & conf = Settings::Get();
//      const RGBA white( 0xFF, 0xFF, 0xFF );
//      const RGBA yellow( 0xFF, 0xFF, 0x00 );
//
//      small
//      fnt_cache[ch].sfs[0] = fonts[0].RenderUnicodeChar( ch, white, !conf.FontSmallRenderBlended() );
//      fnt_cache[ch].sfs[1] = fonts[0].RenderUnicodeChar( ch, yellow, !conf.FontSmallRenderBlended() );
//
//      medium
//      fnt_cache[ch].sfs[2] = fonts[1].RenderUnicodeChar( ch, white, !conf.FontNormalRenderBlended() );
//      fnt_cache[ch].sfs[3] = fonts[1].RenderUnicodeChar( ch, yellow, !conf.FontNormalRenderBlended() );
//
//     DEBUG( DBG_ENGINE, DBG_TRACE, "0x" << std::hex << ch );
// }

void AGG::LoadFNT( void )
{
    // const Settings & conf = Settings::Get();
    //
    // if ( !conf.Unicode() ) {
    //     DEBUG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
    // }
    // else if ( fnt_cache.empty() ) {
    //     const std::string letters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    //     std::vector<u16> unicode = StringUTF8_to_UNICODE( letters );
    //
    //     for ( std::vector<u16>::const_iterator it = unicode.begin(); it != unicode.end(); ++it )
    //         LoadTTFChar( *it );
    //
    //     if ( fnt_cache.empty() ) {
    //         DEBUG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
    //     }
    //     else {
    //         DEBUG( DBG_ENGINE, DBG_INFO, "normal fonts " << conf.FontsNormal() );
    //         DEBUG( DBG_ENGINE, DBG_INFO, "small fonts " << conf.FontsSmall() );
    //         DEBUG( DBG_ENGINE, DBG_INFO, "preload english charsets" );
    //     }
    // }
}

u32 AGG::GetFontHeight( bool small )
{
    return small ? fonts[0].Height() : fonts[1].Height();
}

#else
void AGG::LoadFNT( void )
{
    DEBUG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
}
#endif

// This exists to avoid exposing AGG::ReadChunk
std::vector<u8> AGG::LoadBINFRM( const char * frm_file )
{
    DEBUG( DBG_ENGINE, DBG_TRACE, frm_file );
    return AGG::ReadChunk( frm_file );
}

void AGG::ResetMixer( void )
{
    Mixer::Reset();
    loop_sounds.clear();
    loop_sounds.reserve( 7 );
}

bool AGG::Init( void )
{
    // read data dir
    if ( !ReadDataDir() ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "data files not found" );

#ifdef WITH_ZLIB
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Image & image = CreateImageFromZlib( 288, 200, errorMessage, sizeof( errorMessage ) );

        display.fill( 0 );
        fheroes2::Copy( image, 0, 0, display, ( display.width() - image.width() ) / 2, ( display.height() - image.height() ) / 2, image.width(), image.height() );

        LocalEvent & le = LocalEvent::Get();
        while ( le.HandleEvents() && !le.KeyPress() && !le.MouseClickLeft() )
            ;
#endif

        return false;
    }

#ifdef WITH_TTF
    Settings & conf = Settings::Get();
    const std::string prefix_fonts = System::ConcatePath( "files", "fonts" );
    const std::string font1 = Settings::GetLastFile( prefix_fonts, conf.FontsNormal() );
    const std::string font2 = Settings::GetLastFile( prefix_fonts, conf.FontsSmall() );

    fonts = new FontTTF[2];

    if ( conf.Unicode() ) {
        DEBUG( DBG_ENGINE, DBG_INFO, "fonts: " << font1 << ", " << font2 );
        if ( !fonts[1].Open( font1, conf.FontsNormalSize() ) || !fonts[0].Open( font2, conf.FontsSmallSize() ) )
            conf.SetUnicode( false );
    }
#endif

    // load font
    LoadFNT();

    return true;
}

void AGG::Quit( void )
{
    wav_cache.clear();
    mid_cache.clear();
    loop_sounds.clear();
    // fnt_cache.clear();

#ifdef WITH_TTF
    delete[] fonts;
#endif
}

namespace fheroes2
{
    namespace AGG
    {
        std::vector<std::vector<fheroes2::Sprite> > _icnVsSprite;
        const fheroes2::Sprite errorICNImage;

        std::vector<std::vector<std::vector<fheroes2::Image> > > _tilVsImage;
        const fheroes2::Image errorTILImage;

        const uint32_t headerSize = 6;

        std::map<int, std::vector<fheroes2::Sprite> > _icnVsScaledSprite;

        bool IsValidICNId( int id )
        {
            if ( id < 0 ) {
                return false;
            }

            if ( _icnVsSprite.empty() ) {
                _icnVsSprite.resize( ICN::LASTICN );
            }

            return static_cast<size_t>( id ) < _icnVsSprite.size();
        }

        bool IsValidTILId( int id )
        {
            if ( id < 0 ) {
                return false;
            }

            if ( _tilVsImage.empty() ) {
                _tilVsImage.resize( TIL::LASTTIL );
            }

            return static_cast<size_t>( id ) < _tilVsImage.size();
        }

        void LoadOriginalICN( int id )
        {
            const std::vector<u8> & body = ::AGG::ReadChunk( ICN::GetString( id ) );

            if ( body.empty() ) {
                return;
            }

            StreamBuf imageStream( body );

            const uint32_t count = imageStream.getLE16();
            const uint32_t blockSize = imageStream.getLE32();
            if ( count == 0 || blockSize == 0 ) {
                return;
            }

            _icnVsSprite[id].resize( count );

            for ( uint32_t i = 0; i < count; ++i ) {
                imageStream.seek( headerSize + i * 13 );

                ICNHeader header1;
                imageStream >> header1;

                uint32_t sizeData = 0;
                if ( i + 1 != count ) {
                    ICNHeader header2;
                    imageStream >> header2;
                    sizeData = header2.offsetData - header1.offsetData;
                }
                else {
                    sizeData = blockSize - header1.offsetData;
                }

                Sprite & sprite = _icnVsSprite[id][i];

                sprite.resize( header1.width, header1.height );
                sprite.reset();
                sprite.setPosition( static_cast<int16_t>( header1.offsetX ), static_cast<int16_t>( header1.offsetY ) );

                uint8_t * imageData = sprite.image();
                uint8_t * imageTransform = sprite.transform();

                uint32_t posX = 0;
                const uint32_t width = sprite.width();

                const uint8_t * data = body.data() + headerSize + header1.offsetData;
                const uint8_t * dataEnd = data + sizeData;

                while ( 1 ) {
                    if ( 0 == *data ) { // 0x00 - end line
                        imageData += width;
                        imageTransform += width;
                        posX = 0;
                        ++data;
                    }
                    else if ( 0x80 > *data ) { // 0x7F - count data
                        uint32_t c = *data;
                        ++data;
                        while ( c-- && data != dataEnd ) {
                            imageData[posX] = *data;
                            imageTransform[posX] = 0;
                            ++posX;
                            ++data;
                        }
                    }
                    else if ( 0x80 == *data ) { // 0x80 - end data
                        break;
                    }
                    else if ( 0xC0 > *data ) { // 0xBF - skip data
                        posX += *data - 0x80;
                        ++data;
                    }
                    else if ( 0xC0 == *data ) { // 0xC0 - transform layer
                        ++data;

                        const uint8_t transformValue = *data;
                        const uint8_t transformType = static_cast<uint8_t>( ( ( transformValue & 0x3C ) << 6 ) / 256 + 2 ); // 1 is for skipping

                        uint32_t c = *data % 4 ? *data % 4 : *( ++data );

                        if ( ( transformValue & 0x40 ) && ( transformType <= 15 ) ) {
                            while ( c-- ) {
                                imageTransform[posX] = transformType;
                                ++posX;
                            }
                        }
                        else {
                            posX += c;
                        }

                        ++data;
                    }
                    else if ( 0xC1 == *data ) { // 0xC1
                        ++data;
                        uint32_t c = *data;
                        ++data;
                        while ( c-- ) {
                            imageData[posX] = *data;
                            imageTransform[posX] = 0;
                            ++posX;
                        }
                        ++data;
                    }
                    else {
                        uint32_t c = *data - 0xC0;
                        ++data;
                        while ( c-- ) {
                            imageData[posX] = *data;
                            imageTransform[posX] = 0;
                            ++posX;
                        }
                        ++data;
                    }

                    if ( data >= dataEnd ) {
                        break;
                    }
                }
            }
        }

        // Helper function for LoadModifiedICN
        void CopyICNWithPalette( int icnId, int originalIcnId, int paletteType )
        {
            GetICN( originalIcnId, 0 ); // always avoid calling LoadOriginalICN directly

            _icnVsSprite[icnId] = _icnVsSprite[originalIcnId];
            const std::vector<uint8_t> & palette = PAL::GetPalette( paletteType );
            for ( size_t i = 0; i < _icnVsSprite[icnId].size(); ++i ) {
                ApplyPalette( _icnVsSprite[icnId][i], palette );
            }
        }

        bool LoadModifiedICN( int id )
        {
            switch ( id ) {
            case ICN::ROUTERED:
                CopyICNWithPalette( id, ICN::ROUTE, PAL::RED );
                return true;
            case ICN::FONT:
                LoadOriginalICN( id );
                // The original images contain an issue: image layer has value 50 which is '2' in UTF-8. We must correct these (only 3) places
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    ReplaceColorIdByTransformId( _icnVsSprite[id][i], 50, 2 );
                }
                return true;
            case ICN::YELLOW_FONT:
                CopyICNWithPalette( id, ICN::FONT, PAL::YELLOW_TEXT );
                return true;
            case ICN::YELLOW_SMALFONT:
                CopyICNWithPalette( id, ICN::SMALFONT, PAL::YELLOW_TEXT );
                return true;
            case ICN::GRAY_FONT:
                CopyICNWithPalette( id, ICN::FONT, PAL::GRAY_TEXT );
                return true;
            case ICN::GRAY_SMALL_FONT:
                CopyICNWithPalette( id, ICN::SMALFONT, PAL::GRAY_TEXT );
                return true;
            case ICN::BTNBATTLEONLY:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < static_cast<uint32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::BTNNEWGM, 6 + i );
                    // clean the button
                    Image uniform( 83, 23 );
                    uniform.fill( ( i == 0 ) ? GetColorId( 216, 184, 152 ) : GetColorId( 184, 136, 96 ) );
                    Copy( uniform, 0, 0, out, 28, 18, uniform.width(), uniform.height() );
                    // add 'ba'
                    Blit( GetICN( ICN::BTNCMPGN, i ), 41 - i, 28, out, 30 - i, 13, 28, 14 );
                    // add 'tt'
                    Blit( GetICN( ICN::BTNNEWGM, i ), 25 - i, 13, out, 57 - i, 13, 13, 14 );
                    Blit( GetICN( ICN::BTNNEWGM, i ), 25 - i, 13, out, 70 - i, 13, 13, 14 );
                    // add 'le'
                    Blit( GetICN( ICN::BTNNEWGM, 6 + i ), 97 - i, 21, out, 83 - i, 13, 13, 14 );
                    Blit( GetICN( ICN::BTNNEWGM, 6 + i ), 86 - i, 21, out, 96 - i, 13, 13, 14 );
                    // add 'on'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 44 - i, 21, out, 40 - i, 28, 31, 14 );
                    // add 'ly'
                    Blit( GetICN( ICN::BTNHOTST, i ), 47 - i, 21, out, 71 - i, 28, 12, 13 );
                    Blit( GetICN( ICN::BTNHOTST, i ), 72 - i, 21, out, 84 - i, 28, 13, 13 );
                }
                return true;
            case ICN::BTNMIN:
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < static_cast<uint32_t>( _icnVsSprite[id].size() ); ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::RECRUIT, 4 + i );
                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 30, 4, 31, 15 );
                    // add 'IN'
                    Blit( GetICN( ICN::APANEL, 4 + i ), 23, 20, out, 30, 4, 25, 15 );
                }
                return true;
            case ICN::SPELLS:
                LoadOriginalICN( id );
                _icnVsSprite[id].resize( 66 );
                for ( uint32_t i = 60; i < 66; ++i ) {
                    int originalIndex = 0;
                    if ( i == 60 ) // Mass Cure
                        originalIndex = 6;
                    else if ( i == 61 ) // Mass Haste
                        originalIndex = 14;
                    else if ( i == 62 ) // Mass Slow
                        originalIndex = 1;
                    else if ( i == 63 ) // Mass Bless
                        originalIndex = 7;
                    else if ( i == 64 ) // Mass Curse
                        originalIndex = 3;
                    else if ( i == 65 ) // Mass Shield
                        originalIndex = 15;

                    const Sprite & originalImage = _icnVsSprite[id][originalIndex];
                    Sprite & image = _icnVsSprite[id][i];

                    image.resize( originalImage.width() + 8, originalImage.height() + 8 );
                    image.setPosition( originalImage.x() + 4, originalImage.y() + 4 );
                    image.fill( 1 );

                    AlphaBlit( originalImage, image, 0, 0, 128 );
                    AlphaBlit( originalImage, image, 4, 4, 192 );
                    Blit( originalImage, image, 8, 8 );

                    AddTransparency( image, 1 );
                }
                return true;
            case ICN::CSLMARKER:
                _icnVsSprite[id].resize( 3 );
                for ( uint32_t i = 0; i < 3; ++i ) {
                    _icnVsSprite[id][i] = GetICN( ICN::LOCATORS, 24 );
                    if ( i == 1 ) {
                        ReplaceColorId( _icnVsSprite[id][i], 0x0A, 0xD6 );
                    }
                    else if ( i == 2 ) {
                        ReplaceColorId( _icnVsSprite[id][i], 0x0A, 0xDE );
                    }
                }
                return true;
            case ICN::BATTLESKIP: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TEXTBAR, 4 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 3, 8, out, 3, 1, 43, 14 );

                    // add 'skip'
                    Blit( GetICN( ICN::TEXTBAR, i ), 3, 10, out, 3, 0, 43, 14 );
                }
                return true;
            case ICN::BATTLEWAIT: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TEXTBAR, 4 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 3, 8, out, 3, 1, 43, 14 );

                    // add 'wait'
                    const Sprite wait = Crop( GetICN( ICN::ADVBTNS, 8 + i ), 5, 4, 28, 28 );
                    Image resizedWait( wait.width() / 2, wait.height() / 2 );
                    Resize( wait, resizedWait );

                    Blit( resizedWait, 0, 0, out, ( out.width() - 14 ) / 2, 0, 14, 14 );
                }
                return true;
            case ICN::BUYMAX: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::WELLXTRA, i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 6, 2, 52, 14 );

                    // add 'max'
                    Blit( GetICN( ICN::RECRUIT, 4 + i ), 12, 6, out, 7, 3, 50, 12 );
                }
                return true;
            case ICN::BTNGIFT_GOOD: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TRADPOST, 17 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 6, 4, 72, 15 );

                    // add 'G'
                    Blit( GetICN( ICN::CPANEL, i ), 18 - i, 27, out, 20 - i, 4, 15, 15 );

                    // add 'I'
                    Blit( GetICN( ICN::APANEL, 4 + i ), 22 - i, 20, out, 36 - i, 4, 9, 15 );

                    // add 'F'
                    Blit( GetICN( ICN::APANEL, 4 + i ), 48 - i, 20, out, 46 - i, 4, 13, 15 );

                    // add 'T'
                    Blit( GetICN( ICN::CPANEL, 6 + i ), 59 - i, 21, out, 60 - i, 5, 14, 14 );
                }
                return true;
            case ICN::BTNGIFT_EVIL: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::TRADPOSE, 17 + i );

                    // clean the button
                    Blit( GetICN( ICN::SYSTEME, 11 + i ), 10, 6, out, 6, 4, 72, 15 );

                    // add 'G'
                    Blit( GetICN( ICN::CPANELE, i ), 18 - i, 27, out, 20 - i, 4, 15, 15 );

                    // add 'I'
                    Blit( GetICN( ICN::APANELE, 4 + i ), 22 - i, 20, out, 36 - i, 4, 9, 15 );

                    // add 'F'
                    Blit( GetICN( ICN::APANELE, 4 + i ), 48 - i, 20, out, 46 - i, 4, 13, 15 );

                    // add 'T'
                    Blit( GetICN( ICN::CPANELE, 6 + i ), 59 - i, 21, out, 60 - i, 5, 14, 14 );
                }
                return true;
            case ICN::BTNCONFIG: // a button
                _icnVsSprite[id].resize( 2 );
                for ( uint32_t i = 0; i < 2; ++i ) {
                    Sprite & out = _icnVsSprite[id][i];
                    out = GetICN( ICN::REQUESTS, 1 + i );

                    // add 'config'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 31 - i, 20, out, 10 - i, 4, 77, 16 );
                }
                return true;
            case ICN::PHOENIX:
                LoadOriginalICN( id );
                // First sprite has cropped shadow. We copy missing part from another 'almost' identical frame
                if ( _icnVsSprite[id].size() >= 32 ) {
                    const Sprite & in = _icnVsSprite[id][32];
                    Copy( in, 60, 73, _icnVsSprite[id][1], 60, 73, 14, 13 );
                    Copy( in, 56, 72, _icnVsSprite[id][30], 56, 72, 18, 9 );
                }
                return true;
            case ICN::MONH0028: // phoenix
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 1 ) {
                    const Sprite & correctFrame = GetICN( ICN::PHOENIX, 32 );
                    Copy( correctFrame, 60, 73, _icnVsSprite[id][0], 58, 70, 14, 13 );
                }
                return true;
            case ICN::CAVALRYR:
                LoadOriginalICN( id );
                // Sprite 23 has incorrect colors, we need to replace them
                if ( _icnVsSprite[id].size() >= 23 ) {
                    Sprite & out = _icnVsSprite[id][23];

                    std::vector<uint8_t> indexes( 256 );
                    for ( uint32_t i = 0; i < 256; ++i ) {
                        indexes[i] = static_cast<uint8_t>( i );
                    }

                    indexes[69] = 187;
                    indexes[71] = 195;
                    indexes[73] = 188;
                    indexes[74] = 190;
                    indexes[75] = 193;
                    indexes[76] = 191;
                    indexes[77] = 195;
                    indexes[80] = 195;
                    indexes[81] = 196;
                    indexes[83] = 196;
                    indexes[84] = 197;
                    indexes[151] = 197;

                    ApplyPalette( out, indexes );
                }
                return true;
            case ICN::TROLLMSL:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 1 ) {
                    Sprite & out = _icnVsSprite[id][0];
                    // The original sprite contains 2 pixels which are empty
                    if ( out.width() * out.height() > 188 && out.transform()[147] == 1 && out.transform()[188] == 1 ) {
                        out.transform()[147] = 0;
                        out.image()[147] = 22;

                        out.transform()[188] = 0;
                        out.image()[188] = 24;
                    }
                }
                return true;
            case ICN::TROLL2MSL:
                LoadOriginalICN( ICN::TROLLMSL );
                if ( _icnVsSprite[ICN::TROLLMSL].size() == 1 ) {
                    _icnVsSprite[id].resize( 1 );

                    Sprite & out = _icnVsSprite[id][0];
                    out = _icnVsSprite[ICN::TROLLMSL][0];

                    // The original sprite contains 2 pixels which are empty
                    if ( out.width() * out.height() > 188 && out.transform()[147] == 1 && out.transform()[188] == 1 ) {
                        out.transform()[147] = 0;
                        out.image()[147] = 22;

                        out.transform()[188] = 0;
                        out.image()[188] = 24;
                    }

                    std::vector<uint8_t> indexes( 256 );
                    for ( uint32_t i = 0; i < 256; ++i ) {
                        indexes[i] = static_cast<uint8_t>( i );
                    }

                    indexes[10] = 152;
                    indexes[11] = 153;
                    indexes[12] = 154;
                    indexes[13] = 155;
                    indexes[14] = 155;
                    indexes[15] = 156;
                    indexes[16] = 157;
                    indexes[17] = 158;
                    indexes[18] = 159;
                    indexes[19] = 160;
                    indexes[20] = 160;
                    indexes[21] = 161;
                    indexes[22] = 162;
                    indexes[23] = 163;
                    indexes[24] = 164;
                    indexes[25] = 165;
                    indexes[26] = 166;
                    indexes[27] = 166;
                    indexes[28] = 167;
                    indexes[29] = 168;
                    indexes[30] = 169;
                    indexes[31] = 170;
                    indexes[32] = 171;
                    indexes[33] = 172;
                    indexes[34] = 172;
                    indexes[35] = 173;

                    ApplyPalette( out, indexes );
                }
                return true;
            case ICN::LOCATORE:
            case ICN::LOCATORS:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 15 ) {
                    if ( _icnVsSprite[id][12].width() == 47 ) {
                        Sprite & out = _icnVsSprite[id][12];
                        out = Crop( out, 0, 0, out.width() - 1, out.height() );
                    }
                    if ( _icnVsSprite[id][15].width() == 47 ) {
                        Sprite & out = _icnVsSprite[id][15];
                        out = Crop( out, 0, 0, out.width() - 1, out.height() );
                    }
                }
                return true;
            case ICN::TOWNBKG2:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() == 1 ) {
                    Sprite & out = _icnVsSprite[id][0];
                    // The pixel pixel of the original sprite has a skip value
                    if ( !out.empty() && out.transform()[0] == 1 ) {
                        out.transform()[0] = 0;
                        out.image()[0] = 10;
                    }
                }
                return true;
            case ICN::HSICONS:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 7 ) {
                    Sprite & out = _icnVsSprite[id][7];
                    if ( out.width() == 34 && out.height() == 19 ) {
                        Sprite temp = out;
                        out.resize( out.width() + 1, out.height() );
                        out.reset();
                        Copy( temp, 0, 0, out, 1, 0, temp.width(), temp.height() );
                        Copy( temp, temp.width() - 1, 10, out, 0, 10, 1, 3 );
                    }
                }
                return true;
            case ICN::LISTBOX_EVIL:
                CopyICNWithPalette( id, ICN::LISTBOX, PAL::GRAY );
                for ( size_t i = 0; i < _icnVsSprite[id].size(); ++i ) {
                    ApplyPalette( _icnVsSprite[id][i], 2 );
                }
                return true;
            case ICN::MONS32:
                LoadOriginalICN( id );
                if ( _icnVsSprite[id].size() > 2 ) { // Ranger's sprite
                    const Sprite & source = _icnVsSprite[id][1];
                    Sprite & modified = _icnVsSprite[id][2];
                    Sprite temp( source.width(), source.height() + 1 );
                    temp.reset();
                    Copy( source, 0, 0, temp, 0, 1, source.width(), source.height() );
                    Blit( modified, 0, 0, temp, 1, 0, modified.width(), modified.height() );
                    modified = temp;
                    modified.setPosition( 0, 1 );
                }
                if ( _icnVsSprite[id].size() > 4 ) { // Veteran Pikeman's sprite
                    Sprite & modified = _icnVsSprite[id][4];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = temp;
                    Fill( modified, 7, 0, 4, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 6 ) { // Master Swordsman's sprite
                    Sprite & modified = _icnVsSprite[id][6];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = temp;
                    Fill( modified, 2, 0, 5, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 8 ) { // Champion's sprite
                    Sprite & modified = _icnVsSprite[id][8];

                    Sprite temp( modified.width(), modified.height() + 1 );
                    temp.reset();
                    Blit( modified, 0, 0, temp, 0, 1, modified.width(), modified.height() );
                    modified = temp;
                    Fill( modified, 12, 0, 5, 1, 36 );
                }
                if ( _icnVsSprite[id].size() > 44 ) { // Archimage's sprite
                    Sprite & modified = _icnVsSprite[id][44];
                    Sprite temp = _icnVsSprite[id][43];
                    Blit( modified, 0, 0, temp, 1, 0, modified.width(), modified.height() );
                    modified = temp;
                }
                return true;
            default:
                break;
            }

            return false;
        }

        size_t GetMaximumICNIndex( int id )
        {
            if ( _icnVsSprite[id].empty() ) {
                if ( !LoadModifiedICN( id ) ) {
                    LoadOriginalICN( id );
                }
            }

            return _icnVsSprite[id].size();
        }

        size_t GetMaximumTILIndex( int id )
        {
            if ( _tilVsImage[id].empty() ) {
                _tilVsImage[id].resize( 4 ); // 4 possible sides

                const std::vector<uint8_t> & data = ::AGG::ReadChunk( TIL::GetString( id ) );
                if ( data.size() < headerSize ) {
                    return 0;
                }

                StreamBuf buffer( data );

                const uint32_t count = buffer.getLE16();
                const uint32_t width = buffer.getLE16();
                const uint32_t height = buffer.getLE16();
                const uint32_t size = width * height;
                if ( headerSize + count * size != data.size() ) {
                    return 0;
                }

                std::vector<Image> & originalTIL = _tilVsImage[id][0];

                originalTIL.resize( count );
                for ( uint32_t i = 0; i < count; ++i ) {
                    Image & tilImage = originalTIL[i];
                    tilImage.resize( width, height );
                    memcpy( tilImage.image(), data.data() + headerSize + i * size, size );
                    std::fill( tilImage.transform(), tilImage.transform() + width * height, 0 );
                }

                for ( uint32_t shapeId = 1; shapeId < 4; ++shapeId ) {
                    std::vector<Image> & currentTIL = _tilVsImage[id][shapeId];
                    currentTIL.resize( count );

                    const bool horizontalFlip = ( shapeId & 2 ) != 0;
                    const bool verticalFlip = ( shapeId & 1 ) != 0;

                    for ( uint32_t i = 0; i < count; ++i ) {
                        currentTIL[i] = Flip( originalTIL[i], horizontalFlip, verticalFlip );
                    }
                }
            }

            return _tilVsImage[id][0].size();
        }

        // We have few ICNs which we need to scale like some related to main screen
        bool IsScalableICN( int id )
        {
            return id == ICN::HEROES || id == ICN::BTNSHNGL || id == ICN::SHNGANIM;
        }

        const Sprite & GetScaledICN( int icnId, uint32_t index )
        {
            const Sprite & originalIcn = _icnVsSprite[icnId][index];

            if ( Display::DEFAULT_WIDTH == Display::instance().width() && Display::DEFAULT_HEIGHT == Display::instance().height() ) {
                return originalIcn;
            }

            if ( _icnVsScaledSprite[icnId].empty() ) {
                _icnVsScaledSprite[icnId].resize( _icnVsSprite[icnId].size() );
            }

            Sprite & resizedIcn = _icnVsScaledSprite[icnId][index];

            const double scaleFactorX = static_cast<double>( Display::instance().width() ) / Display::DEFAULT_WIDTH;
            const double scaleFactorY = static_cast<double>( Display::instance().height() ) / Display::DEFAULT_HEIGHT;

            const int32_t resizedWidth = static_cast<int32_t>( originalIcn.width() * scaleFactorX + 0.5 );
            const int32_t resizedHeight = static_cast<int32_t>( originalIcn.height() * scaleFactorY + 0.5 );
            // Resize only if needed
            if ( resizedIcn.width() != resizedWidth || resizedIcn.height() != resizedHeight ) {
                resizedIcn.resize( resizedWidth, resizedHeight );
                resizedIcn.setPosition( static_cast<int32_t>( originalIcn.x() * scaleFactorX + 0.5 ), static_cast<int32_t>( originalIcn.y() * scaleFactorY + 0.5 ) );
                Resize( originalIcn, resizedIcn, false );
            }

            return resizedIcn;
        }

        const Sprite & GetICN( int icnId, uint32_t index )
        {
            if ( !IsValidICNId( icnId ) ) {
                return errorICNImage;
            }

            if ( index >= GetMaximumICNIndex( icnId ) ) {
                return errorICNImage;
            }

            if ( IsScalableICN( icnId ) ) {
                return GetScaledICN( icnId, index );
            }

            return _icnVsSprite[icnId][index];
        }

        uint32_t GetICNCount( int icnId )
        {
            if ( !IsValidICNId( icnId ) ) {
                return 0;
            }

            return static_cast<uint32_t>( GetMaximumICNIndex( icnId ) );
        }

        const Image & GetTIL( int tilId, uint32_t index, uint32_t shapeId )
        {
            if ( shapeId > 3 ) {
                return errorTILImage;
            }

            if ( !IsValidTILId( tilId ) ) {
                return errorTILImage;
            }

            const size_t maxTILIndex = GetMaximumTILIndex( tilId );
            if ( index >= maxTILIndex ) {
                return errorTILImage;
            }

            return _tilVsImage[tilId][shapeId][index];
        }

        const Sprite & GetLetter( uint32_t character, uint32_t fontType )
        {
            if ( character < 0x21 ) {
                return errorICNImage;
            }

            // TODO: correct naming and standartise the code
            switch ( fontType ) {
            case Font::GRAY_BIG:
                return GetICN( ICN::GRAY_FONT, character - 0x20 );
            case Font::GRAY_SMALL:
                return GetICN( ICN::GRAY_SMALL_FONT, character - 0x20 );
            case Font::YELLOW_BIG:
                return GetICN( ICN::YELLOW_FONT, character - 0x20 );
            case Font::YELLOW_SMALL:
                return GetICN( ICN::YELLOW_SMALFONT, character - 0x20 );
            case Font::BIG:
                return GetICN( ICN::FONT, character - 0x20 );
            case Font::SMALL:
                return GetICN( ICN::SMALFONT, character - 0x20 );
            default:
                break;
            }

            return GetICN( ICN::SMALFONT, character - 0x20 );
        }

        const Sprite & GetUnicodeLetter( uint32_t character, uint32_t fontType )
        {
            // TODO: Add Unicode character support
            return GetLetter( character, fontType );
        }

        int32_t GetAbsoluteICNHeight( int icnId )
        {
            const uint32_t frameCount = GetICNCount( icnId );
            if ( frameCount == 0 ) {
                return 0;
            }

            int32_t height = 0;
            for ( uint32_t i = 0; i < frameCount; ++i ) {
                const int32_t offset = -GetICN( icnId, i ).y();
                if ( offset > height ) {
                    height = offset;
                }
            }

            return height;
        }
    }
}
