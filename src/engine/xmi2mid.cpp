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

#include <utility>
#include <iomanip>
#include <fstream>
#include <list>
#include <vector>

#include "engine.h"
#include "system.h"

#define TAG_FORM	0x464F524D
#define TAG_XDIR	0x58444952
#define	TAG_INFO	0x494E464F
#define TAG_CAT0	0x43415420
#define TAG_XMID	0x584D4944
#define TAG_TIMB	0x54494D42
#define TAG_EVNT	0x45564E54
#define TAG_RBRN	0x5242524E
#define TAG_MTHD	0x4D546864
#define TAG_MTRK	0x4D54726B

struct pack_t : public std::pair<u32, u32> /* delta offset */
{
    pack_t() : std::pair<u32, u32>(0, 0) {}
};

std::vector<u8> packValue(u32 delta)
{
    u8 c1 = delta & 0x0000007F;
    u8 c2 = (delta & 0x00003F80) >> 7;
    u8 c3 = (delta & 0x001FC000) >> 14;
    u8 c4 = (delta & 0x0FE00000) >> 21;

    std::vector<u8> res;
    res.reserve(4);

    if(c4)
    {
        res.push_back(c4 | 0x80);
        res.push_back(c3 | 0x80);
        res.push_back(c2 | 0x80);
        res.push_back(c1);
    }
    else
    if(c3)
    {
        res.push_back(c3 | 0x80);
        res.push_back(c2 | 0x80);
        res.push_back(c1);
    }
    else
    if(c2)
    {
        res.push_back(c2 | 0x80);
        res.push_back(c1);
    }
    else
        res.push_back(c1);

    return res;
}

pack_t unpackValue(const u8* ptr)
{
    const u8* p = ptr;
    pack_t res;

    while(*p & 0x80)
    {
        if(4 <= p - ptr)
        {
            ERROR("unpack delta mistake");
            break;
        }

        res.first |= 0x0000007F & *p;
        res.first <<= 7;
        ++p;
    }

    res.first += *p;
    res.second = p - ptr + 1;

    return res;
}

struct meta_t
{
    meta_t() : command(0), quantity(0), duration(0){}
    meta_t(u8 c, u8 q, u32 d) : command(c), quantity(q), duration(d){}

    bool operator< (const meta_t & m) const{ return duration < m.duration; }
    void decrease_duration(u32 delta) { duration -= delta; }

    u8 command;
    u8 quantity;
    u32 duration;
};

struct IFFChunkHeader
{
    u32	ID;      // 4 upper case ASCII chars, padded with 0x20 (space)
    u32	length;  // big-endian

    IFFChunkHeader(u32 id, u32 sz) : ID(id), length(sz) {}
    IFFChunkHeader() : ID(0), length(0) {}

};

StreamBuf & operator>> (StreamBuf & sb, IFFChunkHeader & st)
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    return sb;
}

StreamBuf & operator<< (StreamBuf & sb, const IFFChunkHeader & st)
{
    sb.putBE32(st.ID);
    sb.putBE32(st.length);
    return sb;
}

struct GroupChunkHeader
{
    u32	ID;        // 4 byte ASCII string, either 'FORM', 'CAT ' or 'LIST'
    u32	length;
    u32	type;      // 4 byte ASCII string

    GroupChunkHeader(u32 id, u32 sz, u32 tp) : ID(id), length(sz), type(tp) {}
    GroupChunkHeader() : ID(0), length(0), type(0) {}
};

StreamBuf & operator<< (StreamBuf & sb, const GroupChunkHeader & st)
{
    sb.putBE32(st.ID);
    sb.putBE32(st.length);
    sb.putBE32(st.type);
    return sb;
}

StreamBuf & operator>> (StreamBuf & sb, GroupChunkHeader & st)
{
    st.ID = sb.getBE32();
    st.length = sb.getBE32();
    st.type = sb.getBE32();
    return sb;
}

struct XMITrack
{
    std::vector<u8>	timb;
    std::vector<u8>	evnt;
};

struct XMITracks : std::list<XMITrack>
{
};

struct XMIData
{
    XMITracks	tracks;

    XMIData(const std::vector<u8> & buf)
    {
	StreamBuf sb(buf);

	GroupChunkHeader group;
	IFFChunkHeader iff;

	// FORM XDIR
	sb >> group;
	if(group.ID == TAG_FORM && group.type == TAG_XDIR)
	{
	    // INFO
	    sb >> iff;
	    if(iff.ID == TAG_INFO && iff.length == 2)
	    {
		int numTracks = sb.getLE16();

		// CAT XMID
		sb >> group;
		if(group.ID == TAG_CAT0 && group.type == TAG_XMID)
		{
		    for(int track = 0; track < numTracks; ++track)
		    {
			tracks.push_back(XMITrack());

			std::vector<u8> & timb = tracks.back().timb;
			std::vector<u8> & evnt = tracks.back().evnt;

			sb >> group;
			// FORM XMID
			if(group.ID == TAG_FORM && group.type == TAG_XMID)
			{
			    sb >> iff;
			    // [TIMB]
			    if(iff.ID == TAG_TIMB)
			    {
				timb = sb.getRaw(iff.length);
				if(timb.size() != iff.length)
				{
				    ERROR("parse error: " << "out of range");
				    break;
				}
				sb >> iff;
			    }

			    // [RBRN]
			    if(iff.ID == TAG_RBRN)
			    {
				sb.skip(iff.length);
				sb >> iff;
			    }

			    // EVNT
			    if(iff.ID != TAG_EVNT)
			    {
				ERROR("parse error: " << "evnt");
				break;
			    }

			    evnt = sb.getRaw(iff.length);

			    if(evnt.size() != iff.length)
			    {
				ERROR("parse error: " << "out of range");
				break;
			    }
			}
			else
			    ERROR("unknown tag: " << group.ID << " (expected FORM), " << group.type << " (expected XMID)");
		    }
		}
		else
		    ERROR("parse error: " << "cat xmid");
	    }
	    else
		ERROR("parse error: " << "info");
	}
	else
	    ERROR("parse error: " << "form xdir");
    }

    bool isvalid(void) const
    {
	return !tracks.empty();
    }
};

struct MidEvent
{
    std::vector<u8>	pack;
    u8			data[4]; // status, data1, data2, count
    //char		status;
    //std::vector<u8>	data;

    size_t size(void) const
    {
	return pack.size() + data[3] + 1;
    }

    MidEvent() {}
    MidEvent(u32 delta, u8 st, u8 d1, u8 d2)
    {
	data[0] = st; data[1] = d1; data[2] = d2; data[3] = 2;
	pack = packValue(delta);
    }

    MidEvent(u32 delta, u8 st, u8 d1)
    {
	data[0] = st; data[1] = d1; data[2] = 0; data[3] = 1;
	pack = packValue(delta);
    }
};

StreamBuf & operator<< (StreamBuf & sb, const MidEvent & st)
{
    for(std::vector<u8>::const_iterator
	it = st.pack.begin(); it != st.pack.end(); ++it)
	sb << *it;
    sb << st.data[0];
    if(2 == st.data[3])
	sb << st.data[1] << st.data[2];
    else
    if(1 == st.data[3])
	sb << st.data[1];
    return sb;
}

struct MidEvents : std::list<MidEvent>
{
    size_t count(void) const
    {
	return std::list<MidEvent>::size();
    }

    size_t size(void) const
    {
	size_t res = 0;
	for(const_iterator it = begin(); it != end(); ++it)
	    res += (*it).size();
	return res;
    }

    MidEvents() {}
    MidEvents(const XMITrack & t)
    {
	const u8* ptr = & t.evnt[0];
	const u8* end = ptr + t.evnt.size();

	u32 delta = 0;
	std::list<meta_t> notesoff;

	while(ptr && ptr < end)
	{
    	    // insert event: note off
    	    if(delta)
    	    {
        	// sort duration
        	notesoff.sort();

		std::list<meta_t>::iterator it1 = notesoff.begin();
        	std::list<meta_t>::iterator it2 = notesoff.end();
        	u32 delta2 = 0;

        	// apply delta
        	for(; it1 != it2; ++it1)
        	{
            	    if((*it1).duration <= delta)
            	    {
                	// note off
                	push_back(MidEvent((*it1).duration - delta2, (*it1).command, (*it1).quantity, 0x7F));
                	delta2 += ((*it1).duration - delta2);
            	    }
        	}

        	// remove end notes
        	while(notesoff.size() && notesoff.front().duration <= delta)
            	    notesoff.pop_front();

        	// fixed delta
        	if(delta2) delta -= delta2;

        	// decrease duration
        	for(std::list<meta_t>::iterator
		    it = notesoff.begin(); it != notesoff.end(); it++)
            	    it->decrease_duration(delta);
    	    }

    	    // interval
    	    if(*ptr < 128)
    	    {
        	delta += *ptr;
        	++ptr;
    	    }
    	    else
    	    // command
    	    {
        	// end
        	if(0xFF == *ptr && 0x2F == *(ptr + 1))
        	{
            	    push_back(MidEvent(delta, *ptr, *(ptr + 1), *(ptr + 2)));
            	    break;
        	}
        	else
        	switch(*ptr >> 4)
        	{
            	    // meta
            	    case 0x0F:
            	    {
			pack_t pack = unpackValue(ptr + 2);
			ptr += pack.first + pack.second + 1;
                	delta = 0;
            	    }
            	    break;

            	    // key pressure
            	    case 0x0A:
            	    // control change
            	    case 0x0B:
            	    // pitch bend
            	    case 0x0E:
            	    {
                	push_back(MidEvent(delta, *ptr, *(ptr + 1), *(ptr + 2)));
                	ptr += 3;
                	delta = 0;
            	    }
            	    break;

            	    // note off
            	    case 0x08:
            	    // note on
		    case 0x09:
            	    {
                	push_back(MidEvent(delta, *ptr, *(ptr + 1), *(ptr + 2)));
			pack_t pack = unpackValue(ptr + 3);
                	notesoff.push_back(meta_t(*ptr - 0x10, *(ptr + 1), pack.first));
                	ptr += 3 + pack.second;
                	delta = 0;
            	    }
            	    break;

            	    // program change
            	    case 0x0C:
            	    // chanel pressure
            	    case 0x0D:
            	    {
                	push_back(MidEvent(delta, *ptr, *(ptr + 1)));
                	ptr += 2;
                	delta = 0;
            	    }
            	    break;

            	    // unused command
            	    default:
			push_back(MidEvent(0, 0xFF, 0x2F, 0));
                	ERROR("unknown st: 0x" << std::setw(2) << std::setfill('0') << std::hex <<
			    static_cast<int>(*ptr) << ", ln: " << static_cast<int>(& t.evnt[0] + t.evnt.size() - ptr));
            	    break;
		}
            }
        }
    }
};

StreamBuf & operator<< (StreamBuf & sb, const MidEvents & st)
{
    for(std::list<MidEvent>::const_iterator
	it = st.begin(); it != st.end(); ++it)
	sb << *it;
    return sb;
}

struct MidTrack
{
    IFFChunkHeader	mtrk;
    MidEvents		events;

    MidTrack() : mtrk(TAG_MTRK, 0) {}
    MidTrack(const XMITrack & t) : mtrk(TAG_MTRK, 0), events(t) { mtrk.length = events.size(); }

    size_t size(void) const
    {
	return sizeof(mtrk) + events.size();
    }
};

StreamBuf & operator<< (StreamBuf & sb, const MidTrack & st)
{
    sb << st.mtrk;
    sb << st.events;
    return sb;
}

struct MidTracks : std::list<MidTrack>
{
    size_t count(void) const
    {
	return std::list<MidTrack>::size();
    }

    size_t size(void) const
    {
	size_t res = 0;
	for(const_iterator it = begin(); it != end(); ++it)
	    res += (*it).size();
	return res;
    }

    MidTracks(){}
    MidTracks(const XMITracks & tracks)
    {
	for(XMITracks::const_iterator
	    it = tracks.begin(); it != tracks.end(); ++it)
	    push_back(MidTrack(*it));
    }
};

StreamBuf & operator<< (StreamBuf & sb, const MidTracks & st)
{
    for(std::list<MidTrack>::const_iterator
	it = st.begin(); it != st.end(); ++it)
	sb << *it;
    return sb;
}

struct MidData
{
    IFFChunkHeader	mthd;
    int			format;
    int			ppqn;
    MidTracks		tracks;

    MidData() : mthd(TAG_MTHD, 6), format(0), ppqn(0) {}
    MidData(const XMITracks & t, int p) : mthd(TAG_MTHD, 6), format(0), ppqn(p), tracks(t) {}
};

StreamBuf & operator<< (StreamBuf & sb, const MidData & st)
{
    sb << st.mthd;
    sb.putBE16(st.format);
    sb.putBE16(st.tracks.count());
    sb.putBE16(st.ppqn);
    sb << st.tracks;
    return sb;
}

std::vector<u8> Music::Xmi2Mid(const std::vector<u8> & buf)
{
    XMIData xmi(buf);
    StreamBuf sb(16 * 4096);

    if(xmi.isvalid())
    {
	MidData mid(xmi.tracks, 64);
	sb << mid;
    }

    return std::vector<u8>(sb.data(), sb.data() + sb.size());
}
