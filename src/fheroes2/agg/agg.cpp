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

    struct icn_cache_t
    {
        icn_cache_t()
            : sprites( NULL )
            , reflect( NULL )
            , count( 0 )
        {}

        Sprite * sprites;
        Sprite * reflect;
        u32 count;
    };

    struct til_cache_t
    {
        til_cache_t()
            : sprites( NULL )
            , count( 0 )
        {}

        Surface * sprites;
        u32 count;
    };

    struct fnt_cache_t
    {
        Surface sfs[4]; /* small_white, small_yellow, medium_white, medium_yellow */
    };

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

    std::vector<icn_cache_t> icn_cache;
    std::vector<til_cache_t> til_cache;

    std::map<int, std::vector<u8> > wav_cache;
    std::map<int, std::vector<u8> > mid_cache;
    std::vector<loop_sound_t> loop_sounds;
    std::map<u32, fnt_cache_t> fnt_cache;

    bool memlimit_usage = true;

#ifdef WITH_TTF
    FontTTF * fonts; /* small, medium */

    void LoadTTFChar( u32 );
    Surface GetFNT( u32, u32 );
#endif

    const std::vector<u8> & GetWAV( int m82 );
    const std::vector<u8> & GetMID( int xmi );

    void LoadWAV( int m82, std::vector<u8> & );
    void LoadMID( int xmi, std::vector<u8> & );

    bool LoadExtICN( int icn, u32, bool );
    bool LoadAltICN( int icn, u32, bool );
    bool LoadOrgICN( Sprite &, int icn, u32, bool );
    bool LoadOrgICN( int icn, u32, bool );
    bool LoadICN( int icn, u32, bool reflect = false );
    void SaveICN( int icn );

    bool LoadAltTIL( int til, u32 max );
    bool LoadOrgTIL( int til, u32 max );
    void LoadTIL( int til );
    void SaveTIL( int til );

    void LoadFNT( void );
    void ShowError( void );

    bool CheckMemoryLimit( void );
    u32 ClearFreeObjects( void );

    bool ReadDataDir( void );
    const std::vector<u8> & ReadICNChunk( int icn, u32 );
    const std::vector<u8> & ReadChunk( const std::string & key, bool ignoreExpansion = false );

    // This class is a holder of the original 8-bit image after decompression
    class ICNData
    {
    public:
        explicit ICNData( uint32_t width_ = 0, uint32_t height_ = 0 )
            : _width( 0 )
            , _height( 0 )
        {
            resize( width_, height_ );
        }

        void resize( uint32_t width_ = 0, uint32_t height_ = 0 )
        {
            if ( width_ == _width && height_ == _height )
                return;

            _width = width_;
            _height = height_;

            _data.clear();
            _data.resize( static_cast<size_t>( _width ) * _height, 0u );
        }

        const std::vector<uint8_t> & get() const
        {
            return _data;
        }

        std::vector<uint8_t> & get()
        {
            return _data;
        }

        uint32_t width() const
        {
            return _width;
        }

        uint32_t height() const
        {
            return _height;
        }

    private:
        std::vector<uint8_t> _data;
        uint32_t _width;
        uint32_t _height;
    };

    std::map<std::pair<int, int>, ICNData> _icnIdVsData;
}

Sprite ICNSprite::CreateSprite( bool reflect, bool shadow ) const
{
    Surface res( first.GetSize(), shadow );
    first.Blit( res );

    if ( shadow && second.isValid() )
        second.Blit( res );

    return Sprite( reflect ? res.RenderReflect( 2 ) : res, offset.x, offset.y );
}

bool ICNSprite::isValid( void ) const
{
    return first.isValid();
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

    const u32 size = stream.size();
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

u32 AGG::ClearFreeObjects( void )
{
    u32 total = 0;

    // wav cache
    for ( std::map<int, std::vector<u8> >::iterator it = wav_cache.begin(); it != wav_cache.end(); ++it )
        total += ( *it ).second.size();

    DEBUG( DBG_ENGINE, DBG_INFO,
           "WAV"
               << " "
               << "memory: " << total );
    total = 0;

    // mus cache
    for ( std::map<int, std::vector<u8> >::iterator it = mid_cache.begin(); it != mid_cache.end(); ++it )
        total += ( *it ).second.size();

    DEBUG( DBG_ENGINE, DBG_INFO,
           "MID"
               << " "
               << "memory: " << total );
    total = 0;

#ifdef WITH_TTF
    // fnt cache
    for ( std::map<u32, fnt_cache_t>::iterator it = fnt_cache.begin(); it != fnt_cache.end(); ++it ) {
        total += ( *it ).second.sfs[0].GetMemoryUsage();
        total += ( *it ).second.sfs[1].GetMemoryUsage();
        total += ( *it ).second.sfs[2].GetMemoryUsage();
        total += ( *it ).second.sfs[3].GetMemoryUsage();
    }

    DEBUG( DBG_ENGINE, DBG_INFO,
           "FNT"
               << " "
               << "memory: " << total );
    total = 0;
#endif

    // til cache
    for ( std::vector<til_cache_t>::iterator it = til_cache.begin(); it != til_cache.end(); ++it ) {
        til_cache_t & tils = *it;

        for ( u32 jj = 0; jj < tils.count; ++jj )
            if ( tils.sprites )
                total += tils.sprites[jj].GetMemoryUsage();
    }
    DEBUG( DBG_ENGINE, DBG_INFO,
           "TIL"
               << " "
               << "memory: " << total );
    total = 0;

    // icn cache
    u32 used = 0;

    for ( std::vector<icn_cache_t>::iterator it = icn_cache.begin(); it != icn_cache.end(); ++it ) {
        icn_cache_t & icns = ( *it );

        for ( u32 jj = 0; jj < icns.count; ++jj ) {
            if ( icns.sprites ) {
                Sprite & sprite1 = icns.sprites[jj];

                if ( !sprite1.isRefCopy() ) {
                    total += sprite1.GetMemoryUsage();
                    sprite1.Reset();
                }
                else
                    used += sprite1.GetMemoryUsage();
            }

            if ( icns.reflect ) {
                Sprite & sprite2 = icns.reflect[jj];

                if ( !sprite2.isRefCopy() ) {
                    total += sprite2.GetMemoryUsage();
                    sprite2.Reset();
                }
                else
                    used += sprite2.GetMemoryUsage();
            }
        }
    }

    DEBUG( DBG_ENGINE, DBG_INFO,
           "ICN"
               << " "
               << "memory: " << used );

    return total;
}

bool AGG::CheckMemoryLimit( void )
{
    Settings & conf = Settings::Get();

    // memory limit trigger
    if ( conf.ExtPocketLowMemory() && 0 < conf.MemoryLimit() && memlimit_usage ) {
        u32 usage = System::GetMemoryUsage();

        if ( 0 < usage && conf.MemoryLimit() < usage ) {
            VERBOSE( "settings: " << conf.MemoryLimit() << ", game usage: " << usage );
            const u32 freemem = ClearFreeObjects();
            VERBOSE( "free " << freemem );

            usage = System::GetMemoryUsage();

            if ( conf.MemoryLimit() < usage + ( 300 * 1024 ) ) {
                VERBOSE( "settings: " << conf.MemoryLimit() << ", too small" );
                // increase + 300Kb
                conf.SetMemoryLimit( usage + ( 300 * 1024 ) );
                VERBOSE( "settings: "
                         << "increase limit on 300kb, current value: " << conf.MemoryLimit() );
            }

            return true;
        }
    }

    return false;
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

/* load manual ICN object */
bool AGG::LoadExtICN( int icn, u32 index, bool reflect )
{
    // for animation sprite need update count for ICN::AnimationFrame
    u32 count = 0;
    const Settings & conf = Settings::Get();

    switch ( icn ) {
    case ICN::BOAT12:
        count = 1;
        break;
    case ICN::BATTLESKIP:
    case ICN::BATTLEWAIT:
    case ICN::BATTLEAUTO:
    case ICN::BATTLESETS:
    case ICN::BUYMAX:
    case ICN::BTNBATTLEONLY:
    case ICN::BTNGIFT:
    case ICN::BTNMIN:
    case ICN::BTNCONFIG:
        count = 2;
        break;
    case ICN::CSLMARKER:
        count = 3;
        break;
    case ICN::FONT:
    case ICN::SMALFONT:
    case ICN::YELLOW_FONT:
    case ICN::YELLOW_SMALFONT:
    case ICN::GRAY_FONT:
    case ICN::GRAY_SMALL_FONT:
        count = 96;
        break;
    case ICN::ROUTERED:
        count = 145;
        break;
    case ICN::SPELLS:
        count = 66;

    default:
        break;
    }

    // not modify sprite
    if ( 0 == count )
        return false;

    icn_cache_t & v = icn_cache[icn];
    DEBUG( DBG_ENGINE, DBG_TRACE, ICN::GetString( icn ) << ", " << index );

    if ( NULL == v.sprites ) {
        v.sprites = new Sprite[count];
        v.reflect = new Sprite[count];
        v.count = count;
    }

    // simple modify
    if ( index < count ) {
        Sprite & sprite = reflect ? v.reflect[index] : v.sprites[index];

        memlimit_usage = false;

        switch ( icn ) {
        case ICN::BTNBATTLEONLY:
            LoadOrgICN( sprite, ICN::BTNNEWGM, 2 + index, false );
            // clean
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 10, 6, 55, 14 ), 15, 13, sprite );
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 10, 6, 55, 14 ), 70, 13, sprite );
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 10, 6, 55, 14 ), 42, 28, sprite );
            // ba
            GetICN( ICN::BTNCMPGN, index ).Blit( Rect( 41, 28, 28, 14 ), 30, 13, sprite );
            // tt
            GetICN( ICN::BTNNEWGM, index ).Blit( Rect( 25, 13, 13, 14 ), 57, 13, sprite );
            GetICN( ICN::BTNNEWGM, index ).Blit( Rect( 25, 13, 13, 14 ), 70, 13, sprite );
            // le
            GetICN( ICN::BTNNEWGM, 6 + index ).Blit( Rect( 97, 21, 13, 14 ), 83, 13, sprite );
            GetICN( ICN::BTNNEWGM, 6 + index ).Blit( Rect( 86, 21, 13, 14 ), 96, 13, sprite );
            // on
            GetICN( ICN::BTNDCCFG, 4 + index ).Blit( Rect( 44, 21, 31, 14 ), 40, 28, sprite );
            // ly
            GetICN( ICN::BTNHOTST, index ).Blit( Rect( 47, 21, 13, 13 ), 71, 28, sprite );
            GetICN( ICN::BTNHOTST, index ).Blit( Rect( 72, 21, 13, 13 ), 84, 28, sprite );
            break;

        case ICN::BTNCONFIG:
            LoadOrgICN( sprite, ICN::SYSTEM, 11 + index, false );
            // config
            GetICN( ICN::BTNDCCFG, 4 + index ).Blit( Rect( 30, 20, 80, 16 ), 8, 5, sprite );
            break;

        case ICN::BTNGIFT:
            LoadOrgICN( sprite, ( Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST ), 17 + index, false );
            // clean
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 10, 6, 72, 15 ), 6, 4, sprite );
            // G
            GetICN( ICN::BTNDCCFG, 4 + index ).Blit( Rect( 94, 20, 15, 15 ), 20, 4, sprite );
            // I
            GetICN( ICN::BTNDCCFG, 4 + index ).Blit( Rect( 86, 20, 9, 15 ), 36, 4, sprite );
            // F
            GetICN( ICN::BTNDCCFG, 4 + index ).Blit( Rect( 74, 20, 13, 15 ), 46, 4, sprite );
            // T
            GetICN( ICN::BTNNEWGM, index ).Blit( Rect( 25, 13, 13, 14 ), 60, 5, sprite );
            break;

        case ICN::BTNMIN:
            // max
            LoadOrgICN( sprite, ICN::RECRUIT, index + 4, false );
            // clean
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 10, 6, 31, 15 ), 30, 4, sprite );
            // add: IN
            GetICN( ICN::APANEL, 4 + index ).Blit( Rect( 23, 20, 25, 15 ), 30, 4, sprite );
            break;

        case ICN::BUYMAX:
            LoadOrgICN( sprite, ICN::WELLXTRA, index, false );
            // clean
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 10, 6, 52, 14 ), 6, 2, sprite );
            // max
            GetICN( ICN::RECRUIT, 4 + index ).Blit( Rect( 12, 6, 50, 12 ), 7, 3, sprite );
            break;

        case ICN::BATTLESKIP:
            if ( conf.PocketPC() )
                LoadOrgICN( sprite, ICN::TEXTBAR, index, false );
            else {
                LoadOrgICN( sprite, ICN::TEXTBAR, 4 + index, false );
                // clean
                GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 3, 8, 43, 14 ), 3, 1, sprite );
                // skip
                GetICN( ICN::TEXTBAR, index ).Blit( Rect( 3, 8, 43, 14 ), 3, 0, sprite );
            }
            break;

        case ICN::BATTLEAUTO:
            LoadOrgICN( sprite, ICN::TEXTBAR, 0 + index, false );
            // clean
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 4, 8, 43, 13 ), 3, 10, sprite );
            //
            GetICN( ICN::TEXTBAR, 4 + index ).Blit( Rect( 5, 2, 40, 12 ), 4, 11, sprite );
            break;

        case ICN::BATTLESETS:
            LoadOrgICN( sprite, ICN::TEXTBAR, 0 + index, false );
            // clean
            GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 4, 8, 43, 13 ), 3, 10, sprite );
            //
            GetICN( ICN::ADVBTNS, 14 + index ).Blit( Rect( 5, 5, 26, 26 ), 10, 6, sprite );
            break;

        case ICN::BATTLEWAIT:
            if ( conf.PocketPC() )
                LoadOrgICN( sprite, ICN::ADVBTNS, 8 + index, false );
            else {
                LoadOrgICN( sprite, ICN::TEXTBAR, 4 + index, false );
                // clean
                GetICN( ICN::SYSTEM, 11 + index ).Blit( Rect( 3, 8, 43, 14 ), 3, 1, sprite );
                // wait
                Surface dst = Sprite::ScaleQVGASurface( GetICN( ICN::ADVBTNS, 8 + index ).GetSurface( Rect( 5, 4, 28, 28 ) ) );
                dst.Blit( ( sprite.w() - dst.w() ) / 2, 2, sprite );
            }
            break;

        case ICN::BOAT12: {
            LoadOrgICN( sprite, ICN::ADVMCO, 28 + index, false );
            sprite.SetSurface( Sprite::ScaleQVGASurface( sprite ) );
        } break;

        case ICN::CSLMARKER:
            // sprite: not allow build: complete, not today, all builds (white)
            LoadOrgICN( sprite, ICN::LOCATORS, 24, false );

            // sprite: not allow build: builds requires
            if ( 1 == index )
                sprite.ChangeColorIndex( 0x0A, 0xD6 );
            else
                // sprite: not allow build: lack resources (green)
                if ( 2 == index )
                sprite.ChangeColorIndex( 0x0A, 0xDE );
            break;

        default:
            break;
        }

        memlimit_usage = true;
    }

    // change color
    for ( u32 ii = 0; ii < count; ++ii ) {
        Sprite & sprite = reflect ? v.reflect[ii] : v.sprites[ii];

        switch ( icn ) {
        case ICN::ROUTERED:
            LoadOrgICN( sprite, ICN::ROUTE, ii, false );
            ReplaceColors( sprite, PAL::GetPalette( PAL::RED ), ICN::ROUTE, ii, false );
            break;

        case ICN::FONT:
        case ICN::SMALFONT:
            LoadOrgICN( sprite, icn, ii, false );
            ReplaceColors( sprite, PAL::GetPalette( PAL::WHITE_TEXT ), icn, ii, false );
            break;

        case ICN::YELLOW_FONT:
        case ICN::YELLOW_SMALFONT:
            LoadOrgICN( sprite, icn == ICN::YELLOW_FONT ? ICN::FONT : ICN::SMALFONT, ii, false );
            ReplaceColors( sprite, PAL::GetPalette( PAL::YELLOW_TEXT ), icn == ICN::YELLOW_FONT ? ICN::FONT : ICN::SMALFONT, ii, false );
            break;

        case ICN::GRAY_FONT:
        case ICN::GRAY_SMALL_FONT:
            LoadOrgICN( sprite, icn == ICN::GRAY_FONT ? ICN::FONT : ICN::SMALFONT, ii, false );
            ReplaceColors( sprite, PAL::GetPalette( PAL::GRAY_TEXT ), icn == ICN::GRAY_FONT ? ICN::FONT : ICN::SMALFONT, ii, false );
            break;

        case ICN::SPELLS:
            if ( ii < 60 ) {
                LoadOrgICN( sprite, icn, ii, false );
            }
            else {
                int originalIndex = 0;
                if ( ii == 60 ) // Mass Cure
                    originalIndex = 6;
                else if ( ii == 61 ) // Mass Haste
                    originalIndex = 14;
                else if ( ii == 62 ) // Mass Slow
                    originalIndex = 1;
                else if ( ii == 63 ) // Mass Bless
                    originalIndex = 7;
                else if ( ii == 64 ) // Mass Curse
                    originalIndex = 3;
                else if ( ii == 65 ) // Mass Shield
                    originalIndex = 15;

                Sprite icon = AGG::GetICN( ICN::SPELLS, originalIndex );

                sprite = Sprite( Surface( Size( icon.GetSize().w + 8, icon.GetSize().h + 8 ), icon.amask() ), icon.x() - 4, icon.y() - 4 );
                Sprite icon1( icon.GetSurface(), icon.x(), icon.y() );
                icon1.SetAlphaMod( 128, false );
                icon1.Blit( 0, 0, sprite );

                Sprite icon2( icon.GetSurface(), icon.x(), icon.y() );
                icon2.SetAlphaMod( 192, false );
                icon2.Blit( 4, 4, sprite );

                icon.Blit( 8, 8, sprite );
            }

        default:
            break;
        }
    }

    return true;
}

bool AGG::LoadAltICN( int icn, u32 index, bool reflect )
{
#ifdef WITH_XML
    const std::string prefix_images_icn = System::ConcatePath( System::ConcatePath( "files", "images" ), StringLower( ICN::GetString( icn ) ) );
    const std::string xml_spec = Settings::GetLastFile( prefix_images_icn, "spec.xml" );

    // parse spec.xml
    TiXmlDocument doc;
    const TiXmlElement * xml_icn = NULL;

    if ( doc.LoadFile( xml_spec.c_str() ) && NULL != ( xml_icn = doc.FirstChildElement( "icn" ) ) ) {
        int count, ox, oy;
        xml_icn->Attribute( "count", &count );
        icn_cache_t & v = icn_cache[icn];

        if ( NULL == v.sprites ) {
            v.count = count;
            v.sprites = new Sprite[v.count];
            v.reflect = new Sprite[v.count];
        }

        // find current image
        const TiXmlElement * xml_sprite = xml_icn->FirstChildElement( "sprite" );
        int index1 = index;
        int index2 = 0;

        for ( ; xml_sprite; xml_sprite = xml_sprite->NextSiblingElement( "sprite" ) ) {
            xml_sprite->Attribute( "index", &index2 );
            if ( index1 == index2 )
                break;
        }

        if ( xml_sprite && index2 == index1 ) {
            xml_sprite->Attribute( "ox", &ox );
            xml_sprite->Attribute( "oy", &oy );
            std::string name( xml_spec );
            StringReplace( name, "spec.xml", xml_sprite->Attribute( "name" ) );

            Sprite & sp1 = v.sprites[index];
            Sprite & sp2 = v.reflect[index];

            if ( !sp1.isValid() && System::IsFile( name ) && sp1.Load( name.c_str() ) ) {
                sp1.SetPos( Point( ox, oy ) );
                DEBUG( DBG_ENGINE, DBG_TRACE, xml_spec << ", " << index );
                if ( !reflect )
                    return sp1.isValid();
            }

            if ( reflect && sp1.isValid() && !sp2.isValid() ) {
                sp2 = Sprite( sp1.RenderReflect( 2 ), ox, oy );
                return sp2.isValid();
            }
        }

        DEBUG( DBG_ENGINE, DBG_WARN, "broken xml file: " << xml_spec );
    }
#endif

    return false;
}

void AGG::SaveICN( int icn )
{
#ifdef WITH_XML
#ifdef WITH_DEBUG
    const std::string images_dir = Settings::GetWriteableDir( "images" );

    if ( images_dir.size() ) {
        icn_cache_t & v = icn_cache[icn];

        const std::string icn_lower = StringLower( ICN::GetString( icn ) );
        const std::string icn_dir = System::ConcatePath( images_dir, icn_lower );

        if ( !System::IsDirectory( icn_dir ) )
            System::MakeDirectory( icn_dir );

        if ( System::IsDirectory( icn_dir, true ) ) {
            const std::string stats_file = System::ConcatePath( icn_dir, "spec.xml" );
            bool need_save = false;
            TiXmlDocument doc;
            TiXmlElement * icn_element = NULL;

            if ( doc.LoadFile( stats_file.c_str() ) )
                icn_element = doc.FirstChildElement( "icn" );

            if ( !icn_element ) {
                TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
                doc.LinkEndChild( decl );

                icn_element = new TiXmlElement( "icn" );
                icn_element->SetAttribute( "name", icn_lower.c_str() );
                icn_element->SetAttribute( "count", v.count );

                doc.LinkEndChild( icn_element );
                need_save = true;
            }

            for ( u32 index = 0; index < v.count; ++index ) {
                const Sprite & sp = v.sprites[index];

                if ( sp.isValid() ) {
                    std::ostringstream sp_name;
                    sp_name << std::setw( 3 ) << std::setfill( '0' ) << index;
#ifndef WITH_IMAGE
                    sp_name << ".bmp";
#else
                    sp_name << ".png";
#endif
                    const std::string image_full = System::ConcatePath( icn_dir, sp_name.str() );

                    if ( !System::IsFile( image_full ) ) {
                        sp.Save( image_full );

                        TiXmlElement * sprite_element = new TiXmlElement( "sprite" );
                        sprite_element->SetAttribute( "index", index );
                        sprite_element->SetAttribute( "name", sp_name.str().c_str() );
                        sprite_element->SetAttribute( "ox", sp.x() );
                        sprite_element->SetAttribute( "oy", sp.y() );

                        icn_element->LinkEndChild( sprite_element );

                        need_save = true;
                    }
                }
            }

            if ( need_save )
                doc.SaveFile( stats_file.c_str() );
        }
    }
#endif
#endif
}

const std::vector<u8> & AGG::ReadICNChunk( int icn, u32 index )
{
    // hard fix artifact "ultimate stuff" sprite for loyalty version
    if ( ICN::ARTIFACT == icn && Artifact( Artifact::ULTIMATE_STAFF ).IndexSprite64() == index && heroes2x_agg.isGood() ) {
        return heroes2x_agg.Read( ICN::GetString( icn ) );
    }

    return ReadChunk( ICN::GetString( icn ) );
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

ICNSprite AGG::RenderICNSprite( int icn, u32 index, int palette )
{
    ICNSprite res;
    const std::vector<u8> & body = ReadICNChunk( icn, index );

    if ( body.empty() ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "error: " << ICN::GetString( icn ) );
        return res;
    }

    if ( PAL::CurrentPalette() != palette )
        PAL::SwapPalette( palette );

    // prepare icn data
    DEBUG( DBG_ENGINE, DBG_TRACE, ICN::GetString( icn ) << ", " << index );

    StreamBuf st( body );

    u32 count = st.getLE16();
    u32 blockSize = st.getLE32();
    u32 sizeData = 0;

    if ( index )
        st.skip( index * 13 );

    ICNHeader header1;
    st >> header1;

    if ( index + 1 != count ) {
        ICNHeader header2;
        st >> header2;
        sizeData = header2.offsetData - header1.offsetData;
    }
    else
        sizeData = blockSize - header1.offsetData;

    // start render
    Size sz = Size( header1.width, header1.height );

    const u8 * buf = &body[6 + header1.offsetData];
    const u8 * max = buf + sizeData;

    res.offset = Point( header1.offsetX, header1.offsetY );
    Surface & sf1 = res.first;
    Surface & sf2 = res.second;

    sf1.Set( sz.w, sz.h, false );
    RGBA shadow = RGBA( 0, 0, 0, 0x40 );

    u32 c = 0;
    Point pt( 0, 0 );

    ICNData originalData( sz.w, sz.h );
    uint8_t * icnData = originalData.get().data();

    while ( 1 ) {
        // 0x00 - end line
        if ( 0 == *buf ) {
            ++pt.y;
            pt.x = 0;
            ++buf;
        }
        else
            // 0x7F - count data
            if ( 0x80 > *buf ) {
            c = *buf;
            ++buf;
            while ( c-- && buf < max ) {
                icnData[pt.y * sz.w + pt.x] = *buf;
                sf1.DrawPoint( pt, PAL::GetPaletteColor( *buf ) );
                ++pt.x;
                ++buf;
            }
        }
        else
            // 0x80 - end data
            if ( 0x80 == *buf ) {
            break;
        }
        else
            // 0xBF - skip data
            if ( 0xC0 > *buf ) {
            pt.x += *buf - 0x80;
            ++buf;
        }
        else
            // 0xC0 - shadow
            if ( 0xC0 == *buf ) {
            ++buf;
            c = *buf % 4 ? *buf % 4 : *( ++buf );
            if ( sf1.depth() == 8 ) // skip alpha
            {
                while ( c-- ) {
                    ++pt.x;
                }
            }
            else {
                if ( !sf2.isValid() )
                    sf2.Set( sz.w, sz.h, true );
                while ( c-- ) {
                    icnData[pt.y * sz.w + pt.x] = 0;
                    sf2.DrawPoint( pt, shadow );
                    ++pt.x;
                }
            }
            ++buf;
        }
        else
            // 0xC1
            if ( 0xC1 == *buf ) {
            ++buf;
            c = *buf;
            ++buf;
            while ( c-- ) {
                icnData[pt.y * sz.w + pt.x] = *buf;
                sf1.DrawPoint( pt, PAL::GetPaletteColor( *buf ) );
                ++pt.x;
            }
            ++buf;
        }
        else {
            c = *buf - 0xC0;
            ++buf;
            while ( c-- ) {
                icnData[pt.y * sz.w + pt.x] = *buf;
                sf1.DrawPoint( pt, PAL::GetPaletteColor( *buf ) );
                ++pt.x;
            }
            ++buf;
        }
        if ( buf >= max ) {
            DEBUG( DBG_ENGINE, DBG_WARN, "out of range: " << buf - max );
            break;
        }
    }

    _icnIdVsData[std::make_pair( icn, index )] = originalData;

    if ( icn == ICN::SPELLINL && index == 11 ) { // STONE spell status
        res.second.SetAlphaMod( 0, false );
    }

    return res;
}

bool AGG::LoadOrgICN( Sprite & sp, int icn, u32 index, bool reflect )
{
    ICNSprite icnSprite = AGG::RenderICNSprite( icn, index );

    if ( icnSprite.isValid() ) {
        sp = icnSprite.CreateSprite( reflect, !ICN::SkipLocalAlpha( icn ) );
        return true;
    }

    return false;
}

bool AGG::LoadOrgICN( int icn, u32 index, bool reflect )
{
    icn_cache_t & v = icn_cache[icn];

    if ( NULL == v.sprites ) {
        const std::vector<u8> & body = ReadChunk( ICN::GetString( icn ) );

        if ( body.size() ) {
            v.count = StreamBuf( body ).getLE16();
            if ( v.count < 1 )
                return false;
            v.sprites = new Sprite[v.count];
            v.reflect = new Sprite[v.count];
        }
        else
            return false;
    }

    Sprite & sp = reflect ? v.reflect[index] : v.sprites[index];

    return LoadOrgICN( sp, icn, index, reflect );
}

/* load ICN object */
bool AGG::LoadICN( int icn, u32 index, bool reflect )
{
    icn_cache_t & v = icn_cache[icn];

    // need load
    if ( ( reflect && ( !v.reflect || ( index < v.count && !v.reflect[index].isValid() ) ) )
         || ( !reflect && ( !v.sprites || ( index < v.count && !v.sprites[index].isValid() ) ) ) ) {
        const Settings & conf = Settings::Get();

        // load from images dir
        if ( !conf.UseAltResource() || !LoadAltICN( icn, index, reflect ) ) {
            // load modify sprite
            if ( !LoadExtICN( icn, index, reflect ) ) {
                // load origin sprite
                if ( !LoadOrgICN( icn, index, reflect ) ) {
                    ERROR( "ICN load error: asking for file " << icn << ", sprite index " << index );
                    return false;
                }
            }
#ifdef DEBUG
            if ( Settings::Get().UseAltResource() )
                SaveICN( icn );
#endif
        }

        // pocketpc: scale sprites
        if ( Settings::Get().QVGA() && ICN::NeedMinify4PocketPC( icn, index ) ) {
            Sprite & sp = reflect ? v.reflect[index] : v.sprites[index];
            sp = Sprite::ScaleQVGASprite( sp );
        }
    }
    return true;
}

/* return ICN sprite */
Sprite AGG::GetICN( int icn, u32 index, bool reflect )
{
    Sprite result;

    if ( icn < static_cast<int>( icn_cache.size() ) ) {
        icn_cache_t & v = icn_cache[icn];

        // out of range?
        if ( v.count && index >= v.count ) {
            DEBUG( DBG_ENGINE, DBG_WARN,
                   ICN::GetString( icn ) << ", "
                                         << "out of range: " << index );
            index = 0;
        }

        // need load?
        if ( 0 == v.count || ( ( reflect && ( !v.reflect || !v.reflect[index].isValid() ) ) || ( !v.sprites || !v.sprites[index].isValid() ) ) ) {
            CheckMemoryLimit();
            if ( !LoadICN( icn, index, reflect ) ) {
                return result;
            }
        }

        result = reflect ? v.reflect[index] : v.sprites[index];

        // invalid sprite?
        if ( !result.isValid() ) {
            DEBUG( DBG_ENGINE, DBG_INFO, "invalid sprite: " << ICN::GetString( icn ) << ", index: " << index << ", reflect: " << ( reflect ? "true" : "false" ) );
        }
    }

    return result;
}

/* return count of sprites from specific ICN */
u32 AGG::GetICNCount( int icn )
{
    if ( icn_cache[icn].count == 0 )
        AGG::GetICN( icn, 0 );
    return icn_cache[icn].count;
}

// return height of the biggest frame in specific ICN
int AGG::GetAbsoluteICNHeight( int icn )
{
    int result = 0;

    if ( icn < static_cast<int>( icn_cache.size() ) ) {
        const size_t frameCount = icn_cache[icn].count;
        for ( int i = 0; i < frameCount; ++i ) {
            const int offset = -icn_cache[icn].sprites[i].y();
            if ( offset > result ) {
                result = offset;
            }
        }
    }
    return result;
}

int AGG::PutICN( const Sprite & sprite, bool init_reflect )
{
    icn_cache_t v;

    v.count = 1;
    v.sprites = new Sprite[1];
    v.sprites[0] = sprite;

    if ( init_reflect ) {
        v.reflect = new Sprite[1];
        v.reflect[0] = Sprite( sprite.RenderReflect( 2 ), sprite.x(), sprite.y() );
    }

    icn_cache.push_back( v );
    return icn_cache.size() - 1;
}

bool AGG::LoadAltTIL( int til, u32 max )
{
#ifdef WITH_XML
    const std::string prefix_images_til = System::ConcatePath( System::ConcatePath( "files", "images" ), StringLower( TIL::GetString( til ) ) );
    const std::string xml_spec = Settings::GetLastFile( prefix_images_til, "spec.xml" );

    // parse spec.xml
    TiXmlDocument doc;
    const TiXmlElement * xml_til = NULL;

    if ( doc.LoadFile( xml_spec.c_str() ) && NULL != ( xml_til = doc.FirstChildElement( "til" ) ) ) {
        int count, index;
        xml_til->Attribute( "count", &count );
        til_cache_t & v = til_cache[til];

        if ( NULL == v.sprites ) {
            v.count = count;
            v.sprites = new Surface[v.count];
        }

        index = 0;
        for ( const TiXmlElement * xml_sprite = xml_til->FirstChildElement( "sprite" ); xml_sprite; ++index, xml_sprite = xml_sprite->NextSiblingElement( "sprite" ) ) {
            xml_sprite->Attribute( "index", &index );

            if ( index < count ) {
                Surface & sf = v.sprites[index];
                std::string name( xml_spec );
                StringReplace( name, "spec.xml", xml_sprite->Attribute( "name" ) );

                if ( System::IsFile( name ) )
                    sf.Load( name.c_str() );
                else
                    DEBUG( DBG_ENGINE, DBG_TRACE,
                           "load til"
                               << ": " << name );

                if ( !sf.isValid() )
                    return false;
            }
        }

        return true;
    }
    else
        DEBUG( DBG_ENGINE, DBG_WARN, "broken xml file: " << xml_spec );
#endif

    return false;
}

void AGG::SaveTIL( int til )
{
#ifdef WITH_XML
#ifdef WITH_DEBUG
    const std::string images_dir = Settings::GetWriteableDir( "images" );

    if ( images_dir.size() ) {
        til_cache_t & v = til_cache[til];

        const std::string til_lower = StringLower( ICN::GetString( til ) );
        const std::string til_dir = System::ConcatePath( images_dir, til_lower );

        if ( !System::IsDirectory( til_dir ) )
            System::MakeDirectory( til_dir );

        if ( System::IsDirectory( til_dir, true ) ) {
            const std::string stats_file = System::ConcatePath( til_dir, "spec.xml" );
            bool need_save = false;
            TiXmlDocument doc;
            TiXmlElement * til_element = NULL;

            if ( doc.LoadFile( stats_file.c_str() ) )
                til_element = doc.FirstChildElement( "til" );

            if ( !til_element ) {
                TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
                doc.LinkEndChild( decl );

                til_element = new TiXmlElement( "til" );
                til_element->SetAttribute( "name", til_lower.c_str() );
                til_element->SetAttribute( "count", v.count );

                doc.LinkEndChild( til_element );
                need_save = true;
            }

            for ( u32 index = 0; index < v.count; ++index ) {
                const Surface & sf = v.sprites[index];

                if ( sf.isValid() ) {
                    std::ostringstream sf_name;
                    sf_name << std::setw( 3 ) << std::setfill( '0' ) << index;
#ifndef WITH_IMAGE
                    sf_name << ".bmp";
#else
                    sf_name << ".png";
#endif
                    const std::string image_full = System::ConcatePath( til_dir, sf_name.str() );

                    if ( !System::IsFile( image_full ) ) {
                        sf.Save( image_full );

                        TiXmlElement * sprite_element = new TiXmlElement( "sprite" );
                        sprite_element->SetAttribute( "index", index );
                        sprite_element->SetAttribute( "name", sf_name.str().c_str() );

                        til_element->LinkEndChild( sprite_element );
                        need_save = true;
                    }
                }
            }

            if ( need_save )
                doc.SaveFile( stats_file.c_str() );
        }
    }
#endif
#endif
}

bool AGG::LoadOrgTIL( int til, u32 max )
{
    const std::vector<u8> & body = ReadChunk( TIL::GetString( til ) );

    if ( body.size() ) {
        StreamBuf st( body );

        u32 count = st.getLE16();
        u32 width = st.getLE16();
        u32 height = st.getLE16();

        u32 tile_size = width * height;
        u32 body_size = 6 + count * tile_size;

        til_cache_t & v = til_cache[til];

        // check size
        if ( body.size() == body_size && count <= max ) {
            for ( u32 ii = 0; ii < count; ++ii )
                v.sprites[ii] = Surface( &body[6 + ii * tile_size], width, height, 1, false );

            return true;
        }
        else {
            DEBUG( DBG_ENGINE, DBG_WARN,
                   "size mismach"
                       << ", skipping..." );
        }
    }

    return false;
}

/* load TIL object to AGG::Cache */
void AGG::LoadTIL( int til )
{
    til_cache_t & v = til_cache[til];

    if ( !v.sprites ) {
        DEBUG( DBG_ENGINE, DBG_INFO, TIL::GetString( til ) );
        u32 max = 0;

        switch ( til ) {
        case TIL::CLOF32:
            max = 4;
            break;
        case TIL::GROUND32:
            max = 432;
            break;
        case TIL::STON:
            max = 36;
            break;
        default:
            break;
        }

        v.count = max * 4; // rezerve for rotate sprites
        v.sprites = new Surface[v.count];

        const Settings & conf = Settings::Get();

        // load from images dir
        if ( !conf.UseAltResource() || !LoadAltTIL( til, max ) ) {
            if ( !LoadOrgTIL( til, max ) )
                Error::Except( __FUNCTION__, "load til" );

#ifdef DEBUG
            if ( conf.UseAltResource() )
                SaveTIL( til );
#endif
        }
    }
}

/* return TIL surface from AGG::Cache */
Surface AGG::GetTIL( int til, u32 index, u32 shape )
{
    Surface result;

    if ( til < static_cast<int>( til_cache.size() ) ) {
        til_cache_t & v = til_cache[til];

        if ( 0 == v.count )
            LoadTIL( til );
        u32 index2 = index;

        if ( shape ) {
            switch ( til ) {
            case TIL::STON:
                index2 += 36 * ( shape % 4 );
                break;
            case TIL::CLOF32:
                index2 += 4 * ( shape % 4 );
                break;
            case TIL::GROUND32:
                index2 += 432 * ( shape % 4 );
                break;
            default:
                break;
            }
        }

        if ( index2 >= v.count ) {
            DEBUG( DBG_ENGINE, DBG_WARN,
                   TIL::GetString( til ) << ", "
                                         << "out of range: " << index );
            index2 = 0;
        }

        Surface & surface = v.sprites[index2];

        if ( shape && !surface.isValid() ) {
            const Surface & src = v.sprites[index];

            if ( src.isValid() )
                surface = src.RenderReflect( shape );
            else
                DEBUG( DBG_ENGINE, DBG_WARN, "is NULL" );
        }

        if ( !surface.isValid() ) {
            DEBUG( DBG_ENGINE, DBG_WARN, "invalid sprite: " << TIL::GetString( til ) << ", index: " << index );
        }

        result = surface;
    }

    return result;
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

    DEBUG( DBG_ENGINE, DBG_INFO, M82::GetString( m82 ) );
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
        wavHeader.putLE32( body.size() ); // size

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
    DEBUG( DBG_ENGINE, DBG_INFO, XMI::GetString( xmi ) );
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
                int ch = Mixer::Play( &v[0], v.size(), -1, true );

                if ( 0 <= ch ) {
                    Mixer::Pause( ch );
                    Mixer::Volume( ch, vol * conf.SoundVolume() / 10 );
                    Mixer::Resume( ch );

                    // find unused
                    std::vector<loop_sound_t>::iterator itl = std::find( loop_sounds.begin(), loop_sounds.end(), static_cast<int>( M82::UNKNOWN ) );

                    if ( itl != loop_sounds.end() ) {
                        ( *itl ).sound = m82;
                        ( *itl ).channel = ch;
                    }
                    else
                        loop_sounds.push_back( loop_sound_t( m82, ch ) );

                    DEBUG( DBG_ENGINE, DBG_INFO, M82::GetString( m82 ) );
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
        DEBUG( DBG_ENGINE, DBG_INFO, M82::GetString( m82 ) );
        const std::vector<u8> & v = AGG::GetWAV( m82 );
        int ch = Mixer::Play( &v[0], v.size(), -1, false );
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
    const std::string prefix_music = System::ConcatePath( "files", "music" );
    const MusicSource type = conf.MusicType();

    if ( conf.MusicExt() ) {
        std::string filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus ) );

        if ( !System::IsFile( filename ) )
            filename.clear();

        if ( filename.empty() ) {
            filename = Settings::GetLastFile( prefix_music, MUS::GetString( mus, true ) );

            if ( !System::IsFile( filename ) ) {
                StringReplace( filename, ".ogg", ".mp3" );

                if ( !System::IsFile( filename ) ) {
                    DEBUG( DBG_ENGINE, DBG_WARN, "error read file: " << Settings::GetLastFile( prefix_music, MUS::GetString( mus ) ) << ", skipping..." );
                    filename.clear();
                }
            }
        }

        if ( filename.size() )
            Music::Play( filename, loop );

        DEBUG( DBG_ENGINE, DBG_INFO, MUS::GetString( mus ) );
    }
#ifdef WITH_AUDIOCD
    else if ( type == MUSIC_CDROM && Cdrom::isValid() ) {
        Cdrom::Play( mus, loop );
        DEBUG( DBG_ENGINE, DBG_INFO, "cd track " << static_cast<int>( mus ) );
    }
#endif
    else if ( type == MUSIC_MIDI_EXPANSION || type == MUSIC_MIDI_ORIGINAL ) {
        // Check if music needs to be pulled from HEROES2X
        const int xmi = XMI::FromMUS( mus, type == MUSIC_MIDI_EXPANSION && heroes2x_agg.isGood() );
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
        DEBUG( DBG_ENGINE, DBG_INFO, XMI::GetString( xmi ) );
    }
}

#ifdef WITH_TTF
void AGG::LoadTTFChar( u32 ch )
{
    const Settings & conf = Settings::Get();
    const RGBA white( 0xFF, 0xFF, 0xFF );
    const RGBA yellow( 0xFF, 0xFF, 0x00 );

    // small
    fnt_cache[ch].sfs[0] = fonts[0].RenderUnicodeChar( ch, white, !conf.FontSmallRenderBlended() );
    fnt_cache[ch].sfs[1] = fonts[0].RenderUnicodeChar( ch, yellow, !conf.FontSmallRenderBlended() );

    // medium
    if ( !( conf.QVGA() && !conf.Unicode() ) ) {
        fnt_cache[ch].sfs[2] = fonts[1].RenderUnicodeChar( ch, white, !conf.FontNormalRenderBlended() );
        fnt_cache[ch].sfs[3] = fonts[1].RenderUnicodeChar( ch, yellow, !conf.FontNormalRenderBlended() );
    }

    DEBUG( DBG_ENGINE, DBG_TRACE, "0x" << std::hex << ch );
}

void AGG::LoadFNT( void )
{
    const Settings & conf = Settings::Get();

    if ( !conf.Unicode() ) {
        DEBUG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
    }
    else if ( fnt_cache.empty() ) {
        const std::string letters = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
        std::vector<u16> unicode = StringUTF8_to_UNICODE( letters );

        for ( std::vector<u16>::const_iterator it = unicode.begin(); it != unicode.end(); ++it )
            LoadTTFChar( *it );

        if ( fnt_cache.empty() ) {
            DEBUG( DBG_ENGINE, DBG_INFO, "use bitmap fonts" );
        }
        else {
            DEBUG( DBG_ENGINE, DBG_INFO, "normal fonts " << conf.FontsNormal() );
            DEBUG( DBG_ENGINE, DBG_INFO, "small fonts " << conf.FontsSmall() );
            DEBUG( DBG_ENGINE, DBG_INFO, "preload english charsets" );
        }
    }
}

u32 AGG::GetFontHeight( bool small )
{
    return small ? fonts[0].Height() : fonts[1].Height();
}

/* return letter sprite */
Surface AGG::GetUnicodeLetter( u32 ch, u32 ft )
{
    bool ttf_valid = fonts[0].isValid() && fonts[1].isValid();

    if ( !ttf_valid )
        return GetLetter( ch, ft );

    if ( !fnt_cache[ch].sfs[0].isValid() )
        LoadTTFChar( ch );

    switch ( ft ) {
    case Font::YELLOW_SMALL:
        return fnt_cache[ch].sfs[1];
    case Font::BIG:
        return fnt_cache[ch].sfs[2];
    case Font::YELLOW_BIG:
        return fnt_cache[ch].sfs[3];
    default:
        break;
    }

    return fnt_cache[ch].sfs[0];
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

Surface AGG::GetLetter( u32 ch, u32 ft )
{
    if ( ch < 0x21 )
        DEBUG( DBG_ENGINE, DBG_WARN, "unknown letter" );

    switch ( ft ) {
    case Font::GRAY_BIG:
        return AGG::GetICN( ICN::GRAY_FONT, ch - 0x20 );
    case Font::GRAY_SMALL:
        return AGG::GetICN( ICN::GRAY_SMALL_FONT, ch - 0x20 );
    case Font::YELLOW_BIG:
        return AGG::GetICN( ICN::YELLOW_FONT, ch - 0x20 );
    case Font::YELLOW_SMALL:
        return AGG::GetICN( ICN::YELLOW_SMALFONT, ch - 0x20 );
    case Font::BIG:
        return AGG::GetICN( ICN::FONT, ch - 0x20 );
    case Font::SMALL:
        return AGG::GetICN( ICN::SMALFONT, ch - 0x20 );

    default:
        break;
    }

    return AGG::GetICN( ICN::SMALFONT, ch - 0x20 );
}

void AGG::ResetMixer( void )
{
    Mixer::Reset();
    loop_sounds.clear();
    loop_sounds.reserve( 7 );
}

void AGG::ShowError( void )
{
#ifdef WITH_ZLIB
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Image & image = CreateImageFromZlib( 288, 200, errorMessage, sizeof( errorMessage ) );

    display.fill( 0 );
    fheroes2::Copy( image, 0, 0, display, ( display.width() - image.width() ) / 2, ( display.height() - image.height() ) / 2, image.width(), image.height() );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() && !le.KeyPress() && !le.MouseClickLeft() )
        ;
#endif
}

bool AGG::Init( void )
{
    // read data dir
    if ( !ReadDataDir() ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "data files not found" );
        ShowError();
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

    icn_cache.reserve( ICN::LASTICN + 256 );
    icn_cache.resize( ICN::LASTICN );

    til_cache.resize( TIL::LASTTIL );

    // load palette
    PAL::InitAllPalettes();

    // load font
    LoadFNT();

    return true;
}

void AGG::Quit( void )
{
    for ( std::vector<icn_cache_t>::iterator it = icn_cache.begin(); it != icn_cache.end(); ++it ) {
        icn_cache_t & icns = ( *it );

        if ( icns.sprites )
            delete[] icns.sprites;
        icns.sprites = NULL;
        if ( icns.reflect )
            delete[] icns.reflect;
        icns.reflect = NULL;
    }

    for ( std::vector<til_cache_t>::iterator it = til_cache.begin(); it != til_cache.end(); ++it ) {
        til_cache_t & tils = ( *it );
        if ( tils.sprites )
            delete[] tils.sprites;
    }

    icn_cache.clear();
    til_cache.clear();
    wav_cache.clear();
    mid_cache.clear();
    loop_sounds.clear();
    fnt_cache.clear();
    PAL::Clear();

#ifdef WITH_TTF
    delete[] fonts;
#endif
}

bool AGG::ReplaceColors( Surface & surface, const std::vector<uint8_t> & colorIndexes, int icnId, int icnIndex, bool reflect )
{
    if ( colorIndexes.size() != PALETTE_SIZE )
        return false;

    const std::vector<uint32_t> & rgbColors = PAL::GetRGBColors();

    std::vector<uint32_t> colors( colorIndexes.size() );
    for ( size_t i = 0; i < colorIndexes.size(); ++i )
        colors[i] = rgbColors[colorIndexes[i]];

    return ReplaceColors( surface, colors, icnId, icnIndex, reflect );
}

bool AGG::ReplaceColors( Surface & surface, const std::vector<uint32_t> & rgbColors, int icnId, int icnIndex, bool reflect )
{
    if ( !surface.isValid() || surface.depth() != 32 || rgbColors.size() != PALETTE_SIZE || icnId < 0 || icnIndex < 0 )
        return false;

    std::map<std::pair<int, int>, ICNData>::const_iterator iter = _icnIdVsData.find( std::make_pair( icnId, icnIndex ) );
    if ( iter == _icnIdVsData.end() )
        return false;

    const ICNData & data = iter->second;
    if ( surface.w() != data.width() || surface.h() != data.height() )
        return false;

    return surface.SetColors( data.get(), rgbColors, reflect );
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

        const std::set<int> _scalableICNIds{ICN::HEROES, ICN::BTNSHNGL, ICN::SHNGANIM};
        std::map<int, std::vector<fheroes2::Sprite> > _icnVsScaledSprite;

        void PrepareICNImages()
        {
            if ( _icnVsSprite.empty() ) {
                _icnVsSprite.resize( ICN::LASTICN );
            }
        }

        void PrepareTILImages()
        {
            if ( _tilVsImage.empty() ) {
                _tilVsImage.resize( TIL::LASTTIL );
            }
        }

        bool IsValidICNId( int id )
        {
            if ( id < 0 ) {
                return false;
            }

            PrepareICNImages();

            return static_cast<size_t>( id ) < _icnVsSprite.size();
        }

        bool IsValidTILId( int id )
        {
            if ( id < 0 ) {
                return false;
            }

            PrepareTILImages();

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
                        while ( c-- ) {
                            if ( transformType <= 15 ) {
                                imageTransform[posX] = transformType;
                            }
                            ++posX;
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
                    out = GetICN( ICN::BTNNEWGM, 2 + i );
                    // clean the button
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 15, 13, 55, 14 );
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 70, 13, 55, 14 );
                    Blit( GetICN( ICN::SYSTEM, 11 + i ), 10, 6, out, 42, 28, 55, 14 );
                    // add 'ba'
                    Blit( GetICN( ICN::BTNCMPGN, i ), 41, 28, out, 30, 13, 28, 14 );
                    // add 'tt'
                    Blit( GetICN( ICN::BTNNEWGM, i ), 25, 13, out, 57, 13, 13, 14 );
                    Blit( GetICN( ICN::BTNNEWGM, i ), 25, 13, out, 70, 13, 13, 14 );
                    // add 'le'
                    Blit( GetICN( ICN::BTNNEWGM, 6 + i ), 97, 21, out, 83, 13, 13, 14 );
                    Blit( GetICN( ICN::BTNNEWGM, 6 + i ), 86, 21, out, 96, 13, 13, 14 );
                    // add 'on'
                    Blit( GetICN( ICN::BTNDCCFG, 4 + i ), 44, 21, out, 40, 28, 31, 14 );
                    // add 'ly'
                    Blit( GetICN( ICN::BTNHOTST, i ), 47, 21, out, 71, 28, 13, 13 );
                    Blit( GetICN( ICN::BTNHOTST, i ), 72, 21, out, 84, 28, 13, 13 );
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
                LoadOriginalICN( ICN::SPELLS );
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
                    const Image wait = Crop( GetICN( ICN::ADVBTNS, 8 + i ), 5, 4, 28, 28 );
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
            return std::find( _scalableICNIds.begin(), _scalableICNIds.end(), id ) != _scalableICNIds.end();
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
    }
}
