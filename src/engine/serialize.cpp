/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#include "rect.h"
#include "system.h"
#include "serialize.h"

#define MINCAPACITY 1024

void StreamBase::setconstbuf(bool f)
{
    if(f)
	flags |= 0x00001000;
    else
	flags &= ~0x00001000;
}

bool StreamBase::isconstbuf(void) const
{
    return flags & 0x00001000;
}

bool StreamBase::bigendian(void) const
{
    return flags & 0x80000000;
}

void StreamBase::setbigendian(bool f)
{
    if(f)
	flags |= 0x80000000;
    else
	flags &= ~0x80000000;
}

void StreamBase::setfail(bool f)
{
    if(f)
	flags |= 0x00000001;
    else
	flags &= ~0x00000001;
}

bool StreamBase::fail(void) const
{
    return flags & 0x00000001;
}

int StreamBase::get16(void)
{
    return bigendian() ? getBE16() : getLE16();
}

int StreamBase::get32(void)
{
    return bigendian() ? getBE32() : getLE32();
}


StreamBase & StreamBase::operator>> (bool & v)
{
    v = get8();
    return *this;
}

StreamBase & StreamBase::operator>> (char & v)
{
    v = get8();
    return *this;
}

StreamBase & StreamBase::operator>> (u8 & v)
{
    v = get8();
    return *this;
}

StreamBase & StreamBase::operator>> (s8 & v)
{
    v = get8();
    return *this;
}

StreamBase & StreamBase::operator>> (u16 & v)
{
    v = get16();
    return *this;
}

StreamBase & StreamBase::operator>> (s16 & v)
{
    v = get16();
    return *this;
}

StreamBase & StreamBase::operator>> (u32 & v)
{
    v = get32();
    return *this;
}

StreamBase & StreamBase::operator>> (s32 & v)
{
    v = get32();
    return *this;
}

StreamBase & StreamBase::operator>> (float & v)
{
    s32 intpart;
    s32 decpart;
    *this >> intpart >> decpart;
    v = intpart + decpart / 100000000;
    return *this;
}

StreamBase & StreamBase::operator>> (std::string & v)
{
    u32 size = get32();
    v.resize(size);

    for(std::string::iterator
        it = v.begin(); it != v.end(); ++it) *it = get8();

    return *this;
}

StreamBase & StreamBase::operator>> (Rect & v)
{
    Point & p = v;
    Size  & s = v;

    return *this >> p >> s;
}

StreamBase & StreamBase::operator>> (Point& v)
{
    return *this >> v.x >> v.y;
}

StreamBase & StreamBase::operator>> (Size & v)
{
    return *this >> v.w >> v.h;
}

void StreamBase::put16(u16 v)
{
    bigendian() ? putBE16(v) : putLE16(v);
}

void StreamBase::put32(u32 v)
{
    bigendian() ? putBE32(v) : putLE32(v);
}

StreamBase & StreamBase::operator<< (const bool & v)
{
    put8(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const char & v)
{
    put8(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const u8 & v)
{
    put8(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const s8 & v)
{
    put8(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const u16 & v)
{
    put16(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const s16 & v)
{
    put16(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const s32 & v)
{
    put32(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const u32 & v)
{
    put32(v);
    return *this;
}

StreamBase & StreamBase::operator<< (const float & v)
{
    s32 intpart = static_cast<s32>(v);
    float decpart = (v - intpart) * 100000000;
    return *this << intpart << static_cast<s32>(decpart);
}

StreamBase & StreamBase::operator<< (const std::string & v)
{
    put32(v.size());

    for(std::string::const_iterator
        it = v.begin(); it != v.end(); ++it) put8(*it);

    return *this;
}

StreamBase & StreamBase::operator<< (const Point & v)
{
    return *this << v.x << v.y;
}

StreamBase & StreamBase::operator<< (const Rect & v)
{
    const Point & p = v;
    const Size  & s = v;

    return *this << p << s;
}

StreamBase & StreamBase::operator<< (const Size & v)
{
    return *this << v.w << v.h;
}

StreamBuf::StreamBuf(size_t sz) : itbeg(NULL), itget(NULL), itput(NULL), itend(NULL)
{
    if(sz) realloc(sz);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    setbigendian(true); /* default: hardware endian */
#else
    setbigendian(false); /* default: hardware endian */
#endif
}

StreamBuf::~StreamBuf()
{
    if(itbeg && ! isconstbuf()) delete [] itbeg;
}

StreamBuf::StreamBuf(const StreamBuf & st) : itbeg(NULL), itget(NULL), itput(NULL), itend(NULL)
{
    copy(st);
}

StreamBuf::StreamBuf(const std::vector<u8> & buf) : itbeg(NULL), itget(NULL), itput(NULL), itend(NULL)
{
    itbeg = (u8*) & buf[0];
    itend = itbeg + buf.size();
    itget = itbeg;
    itput = itend;
    setconstbuf(true);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    setbigendian(true); /* default: hardware endian */
#else
    setbigendian(false); /* default: hardware endian */
#endif
}

StreamBuf::StreamBuf(const u8* buf, size_t bufsz) : itbeg(NULL), itget(NULL), itput(NULL), itend(NULL)
{
    itbeg = const_cast<u8*>(buf);
    itend = itbeg + bufsz;
    itget = itbeg;
    itput = itend;
    setconstbuf(true);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    setbigendian(true); /* default: hardware endian */
#else
    setbigendian(false); /* default: hardware endian */
#endif
}

StreamBuf & StreamBuf::operator= (const StreamBuf & st)
{
    if(&st != this) copy(st);
    return *this;
}

size_t StreamBuf::capacity(void) const
{
    return itend - itbeg;
}

const u8* StreamBuf::data(void) const
{
    return itget;
}

size_t StreamBuf::size(void) const
{
    return sizeg();
}

void StreamBuf::reset(void)
{
    itput = itbeg;
    itget = itbeg;
}

size_t StreamBuf::tellg(void) const
{
    return itget - itbeg;
}

size_t StreamBuf::tellp(void) const
{
    return itput - itbeg;
}

size_t StreamBuf::sizeg(void) const
{
    return itput - itget;
}

size_t StreamBuf::sizep(void) const
{
    return itend - itput;
}

void StreamBuf::realloc(size_t sz)
{
    setconstbuf(false);

    if(! itbeg)
    {
	if(sz < MINCAPACITY) sz = MINCAPACITY;

	itbeg = new u8 [sz];
	itend = itbeg + sz;
    	std::fill(itbeg, itend, 0);

	reset();
    }
    else
    if(sizep() < sz)
    {
	if(sz < MINCAPACITY) sz = MINCAPACITY;

	u8* ptr = new u8 [sz];

	std::fill(ptr, ptr + sz, 0);
	std::copy(itbeg, itput, ptr);

	itput = ptr + tellp();
	itget = ptr + tellg();

	delete [] itbeg;

	itbeg = ptr;
	itend = itbeg + sz;
    }
}

void StreamBuf::setfail(void)
{
    flags |= 0x00000001;
}

void StreamBuf::copy(const StreamBuf & sb)
{
    if(capacity() < sb.size())
	realloc(sb.size());

    std::copy(sb.itget, sb.itput, itbeg);

    itput = itbeg + sb.tellp();
    itget = itbeg + sb.tellg();
    flags = 0;

    setbigendian(sb.bigendian());
}


void StreamBuf::put8(char v)
{
    if(0 == sizep())
	realloc(capacity() + capacity() / 2);

    if(sizep())
        *itput++ = v;
}

int StreamBuf::get8(void)
{
    int res = 0;

    if(sizeg())
	res = 0x000000FF & *itget++;

    return res;
}

int StreamBuf::getBE16(void)
{
    return (get8() << 8) | get8();
}

int StreamBuf::getLE16(void)
{
    return get8() | (get8() << 8);
}

int StreamBuf::getBE32(void)
{
    return (get8() << 24) | (get8() << 16) | (get8() << 8) | get8();
}

int StreamBuf::getLE32(void)
{
    return get8() | (get8() << 8) | (get8() << 16) | (get8() << 24);
}

void StreamBuf::putBE16(u16 v)
{
    put8(v >> 8);
    put8(v);
}

void StreamBuf::putLE16(u16 v)
{
    put8(v);
    put8(v >> 8);
}

void StreamBuf::putBE32(u32 v)
{
    put8(v >> 24);
    put8(v >> 16);
    put8(v >> 8);
    put8(v);
}

void StreamBuf::putLE32(u32 v)
{
    put8(v);
    put8(v >> 8);
    put8(v >> 16);
    put8(v >> 24);
}

std::vector<u8> StreamBuf::getRaw(size_t sz)
{
    std::vector<u8> v(sz ? sz : sizeg(), 0);

    for(std::vector<u8>::iterator
        it = v.begin(); it != v.end(); ++it) *this >> *it;

    return v;
}

void StreamBuf::putRaw(const char* ptr, size_t sz)
{
    for(size_t it = 0; it < sz; ++it)
	*this << ptr[it];
}

std::string StreamBuf::toString(size_t sz)
{
    u8* it1 = itget;
    u8* it2 = itget + (sz ? sz : sizeg());
    it2 = std::find(it1, it2, 0);
    itget = it1 + (sz ? sz : sizeg());
    return std::string(it1, it2);
}

void StreamBuf::skip(size_t sz)
{
    itget += sz <= sizeg() ? sz : sizeg();
}

void StreamBuf::seek(size_t sz)
{
    itget = itbeg + sz < itend ? itbeg + sz : itend;
}

/*
std::ostream & operator<< (std::ostream & os, StreamBuf & sb)
{
    const u32 count = sb.sizeg();

    os.unsetf(std::ios::skipws);
    sb.bigendian() ? StreamBase::putBE32(os, count) : StreamBase::putLE32(os, count);

    if(os.write((char*) sb.itget, count))
	sb.itget += count;

    return os;
}

size_t available_count(std::istream & is)
{
    const size_t curpos = is.tellg();
    is.seekg(0, std::ios_base::end);
    const size_t sizeis = is.tellg();
    is.seekg(curpos, std::ios_base::beg);
    return sizeis  > curpos ? sizeis - curpos : 0;
}

std::istream & operator>> (std::istream & is, StreamBuf & sb)
{
    is.unsetf(std::ios::skipws);
    const u32 count = sb.bigendian() ? StreamBase::getBE32(is) : StreamBase::getLE32(is);

    if(count > available_count(is))
    {
	sb.setfail();
	return is;
    }

    if(sb.sizep() < count)
	sb.realloc(count);

    if(is.read((char*) sb.itput, count))
	sb.itput += count;
    else
	sb.setfail();

    return is;
}
*/

StreamFile::StreamFile(const std::string & fn, const char* mode)
{
    open(fn, mode);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    setbigendian(true); /* default: hardware endian */
#else
    setbigendian(false); /* default: hardware endian */
#endif
}

StreamFile::~StreamFile()
{
    close();
}
    
bool StreamFile::open(const std::string & fn, const char* mode)
{   
    rw = SDL_RWFromFile(fn.c_str(), mode);
    if(! rw) ERROR(SDL_GetError());
    return rw;
}

void StreamFile::close(void)
{
    if(rw) SDL_RWclose(rw);
    rw = NULL;
}

size_t StreamFile::size(void) const
{
    if(rw)
    {
	size_t pos = SDL_RWtell(rw);
	SDL_RWseek(rw, 0, RW_SEEK_END);
	size_t len = SDL_RWseek(rw, 0, SEEK_END);
	SDL_RWseek(rw, pos, RW_SEEK_SET);
	return len;
    }
    return 0;
}

size_t StreamFile::tell(void) const
{
    return tellg();
}

void StreamFile::seek(size_t pos)
{
    if(rw) SDL_RWseek(rw, pos, RW_SEEK_SET);
}

size_t StreamFile::sizeg(void) const
{
    if(rw)
    {
	size_t pos = SDL_RWtell(rw);
	size_t len = SDL_RWseek(rw, 0, RW_SEEK_END);
	SDL_RWseek(rw, pos, RW_SEEK_SET);
	return len - pos;
    }
    return 0;
}

size_t StreamFile::tellg(void) const
{
    return rw ? SDL_RWtell(rw) : 0;
}

size_t StreamFile::sizep(void) const
{
    return sizeg();
}

size_t StreamFile::tellp(void) const
{
    return tellg();
}

void StreamFile::skip(size_t pos)
{
    if(rw) SDL_RWseek(rw, pos, RW_SEEK_CUR);
}

int StreamFile::get8(void)
{
    u8 ch;
    if(rw) SDL_RWread(rw, & ch, 1, 1);
    return ch;
}

void StreamFile::put8(char ch)
{
    if(rw) SDL_RWwrite(rw, & ch, 1, 1);
}

int StreamFile::getBE16(void)
{
    return rw ? SDL_ReadBE16(rw) : 0;
}

int StreamFile::getLE16(void)
{
    return rw ? SDL_ReadLE16(rw) : 0;
}

int StreamFile::getBE32(void)
{
    return rw ? SDL_ReadBE32(rw) : 0;
}

int StreamFile::getLE32(void)
{
    return rw ? SDL_ReadLE32(rw) : 0;
}

void StreamFile::putBE32(u32 val)
{
    if(rw) SDL_WriteBE32(rw, val);
}

void StreamFile::putLE32(u32 val)
{
    if(rw) SDL_WriteLE32(rw, val);
}

void StreamFile::putBE16(u16 val)
{
    if(rw) SDL_WriteBE16(rw, val);
}

void StreamFile::putLE16(u16 val)
{
    if(rw) SDL_WriteLE16(rw, val);
}

std::vector<u8> StreamFile::getRaw(size_t sz)
{
    std::vector<u8> v(sz ? sz : sizeg(), 0);
    if(rw) SDL_RWread(rw, & v[0], v.size(), 1);
    return v;
}

void StreamFile::putRaw(const char* ptr, size_t sz)
{
    if(rw) SDL_RWwrite(rw, ptr, sz, 1);
}

StreamBuf StreamFile::toStreamBuf(size_t sz)
{
    StreamBuf sb;
    std::vector<u8> buf = getRaw(sz);
    sb.putRaw(reinterpret_cast<const char*>(& buf[0]), buf.size());
    return sb;
}

std::string StreamFile::toString(size_t sz)
{
    const std::vector<u8> buf = getRaw(sz);
    std::vector<u8>::const_iterator itend = std::find(buf.begin(), buf.end(), 0);
    return std::string(buf.begin(), itend != buf.end() ? itend : buf.end());
}
