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

#include <map>
#include <list>
#include <string>

#include "engine.h"

struct chunk
{
    u32		offset;
    u32		length;

    chunk() : offset(0), length(0) {}
    chunk(u32 off, u32 len) : offset(off), length(len) {}
};

u32 crc32b(const char* msg)
{
    u32 crc = 0xFFFFFFFF;
    u32 index = 0;

    while(msg[index])
    {
       crc ^= static_cast<u32>(msg[index]);

       for(int bit = 0; bit < 8; ++bit)
       {
           size_t mask = -(crc & 1);
           crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }

       ++index;
    }

   return ~crc;
}

struct mofile
{
    u32 count, offset_strings1, offset_strings2, hash_size, hash_offset;
    StreamBuf buf;
    std::map<u32, chunk> hash_offsets;
    std::string encoding;
    std::string plural_forms;
    u32 nplurals;

    mofile() : count(0), offset_strings1(0), offset_strings2(0), hash_size(0), hash_offset(0), nplurals(0) {}

    const char* ngettext(const char* str, size_t plural)
    {
	std::map<u32, chunk>::const_iterator it = hash_offsets.find(crc32b(str));
	if(it == hash_offsets.end())
	    return str;

	buf.seek((*it).second.offset);
	const u8* ptr = buf.data();

	while(plural > 0)
	{
	    while(*ptr) ptr++;
	    plural--;
	    ptr++;
	}

	return reinterpret_cast<const char*>(ptr);
    }

    std::string get_tag(const std::string & str, const std::string & tag, const std::string & sep)
    {
	std::string res;
	if(str.size() > tag.size() &&
	    tag == str.substr(0, tag.size()))
	{
	    size_t pos = str.find(sep);
	    if(pos != std::string::npos)
	    res = str.substr(pos + sep.size());
	}
	return res;
    }

    bool open(const std::string & file)
    {
	StreamFile sf;

	if(! sf.open(file, "rb"))
	    return false;
	else
	{
	    u32 size = sf.size();
	    u32 id = 0;
	    sf >> id;

	    if(0x950412de != id)
	    {
		ERROR("incorrect mo id: " << GetHexString(id));
		return false;
	    }
	    else
	    {
		u16 major, minor;
		sf >> major >> minor;

		if(0 != major)
		{
		    ERROR("incorrect major version: " << GetHexString(major, 4));
		    return false;
		}
		else
		{
		    sf >> count >> offset_strings1 >> offset_strings2 >> hash_size >> hash_offset;


		    sf.seek(0);
		    buf = sf.toStreamBuf(size);
		    sf.close();
		}
	    }
	}

	// parse encoding and plural forms
	if(count)
	{
	    buf.seek(offset_strings2);
	    u32 length2 = buf.get32();
	    u32 offset2 = buf.get32();

	    const std::string tag1("Content-Type");
	    const std::string sep1("charset=");
	    const std::string tag2("Plural-Forms");
	    const std::string sep2(": ");

	    buf.seek(offset2);
	    std::list<std::string> tags = StringSplit(buf.toString(length2), "\n");

	    for(std::list<std::string>::const_iterator
		it = tags.begin(); it != tags.end(); ++it)
	    {
		if(encoding.empty())
		    encoding = get_tag(*it, tag1, sep1);

		if(plural_forms.empty())
		    plural_forms = get_tag(*it, tag2, sep2);
	    }
	}

	// generate hash table
	for(u32 index = 0; index < count; ++index)
	{
	    buf.seek(offset_strings1 + index * 8 /* length, offset */);
	    u32 length1 = buf.get32();
	    u32 offset1 = buf.get32();
	    buf.seek(offset1);
	    const std::string msg1 = buf.toString(length1);
	    u32 crc = crc32b(msg1.c_str());
	    buf.seek(offset_strings2 + index * 8 /* length, offset */);
	    u32 length2 = buf.get32();
	    u32 offset2 = buf.get32();
	    std::map<u32, chunk>::const_iterator it = hash_offsets.find(crc);
	    if(it == hash_offsets.end())
		hash_offsets[crc] = chunk(offset2, length2);
	    else
	    {
		ERROR("incorrect hash for: " << msg1);
	    }
	}

        return true;
    }

};

namespace Translation
{
    enum { LOCALE_EN, LOCALE_AF, LOCALE_AR, LOCALE_BG, LOCALE_CA, LOCALE_CS, LOCALE_DA, LOCALE_DE, LOCALE_EL, LOCALE_ES,
	    LOCALE_ET, LOCALE_EU, LOCALE_FI, LOCALE_FR, LOCALE_GL, LOCALE_HE, LOCALE_HR, LOCALE_HU, LOCALE_ID, LOCALE_IT,
	    LOCALE_LA, LOCALE_LT, LOCALE_LV, LOCALE_MK, LOCALE_NL, LOCALE_PL, LOCALE_PT, LOCALE_RU, LOCALE_SK, LOCALE_SL,
	    LOCALE_SR, LOCALE_SV, LOCALE_TR };

    mofile*				current = NULL;
    std::map<std::string, mofile>	domains;
    int					locale = LOCALE_EN;
    char				context = 0;

    void setStripContext(char strip)
    {
	context = strip;
    }

    const char* stripContext(const char* str)
    {
	if(! context) return str;
	const char* pos = str;
	while(*pos && *pos++ != context);
	return *pos ? pos : str;
    }

    bool bindDomain(const char* domain, const char* file)
    {
	std::map<std::string, mofile>::const_iterator it = domains.find(domain);
	if(it != domains.end())
	    return true;

	std::string str = System::GetMessageLocale(1);

	if(str == "af" || str == "afrikaans")	locale = LOCALE_AF;
	else
	if(str == "ar" || str == "arabic")	locale = LOCALE_AR;
	else
	if(str == "bg" || str == "bulgarian")	locale = LOCALE_BG;
	else
	if(str == "ca" || str == "catalan")	locale = LOCALE_CA;
	else
	if(str == "da" || str == "danish")	locale = LOCALE_DA;
	else
	if(str == "de" || str == "german")	locale = LOCALE_DE;
	else
	if(str == "el" || str == "greek")	locale = LOCALE_EL;
	else
	if(str == "es" || str == "spanish")	locale = LOCALE_ES;
	else
	if(str == "et" || str == "estonian")	locale = LOCALE_ET;
	else
	if(str == "eu" || str == "basque")	locale = LOCALE_EU;
	else
	if(str == "fi" || str == "finnish")	locale = LOCALE_FI;
	else
	if(str == "fr" || str == "french")	locale = LOCALE_FR;
	else
	if(str == "gl" || str == "galician")	locale = LOCALE_GL;
	else
	if(str == "he" || str == "hebrew")	locale = LOCALE_HE;
	else
	if(str == "hr" || str == "croatian")	locale = LOCALE_HR;
	else
	if(str == "hu" || str == "hungarian")	locale = LOCALE_HU;
	else
	if(str == "id" || str == "indonesian")	locale = LOCALE_ID;
	else
	if(str == "it" || str == "italian")	locale = LOCALE_IT;
	else
	if(str == "la" || str == "latin")	locale = LOCALE_LA;
	else
	if(str == "lt" || str == "lithuanian")	locale = LOCALE_LT;
	else
	if(str == "lv" || str == "latvian")	locale = LOCALE_LV;
	else
	if(str == "mk" || str == "macedonia")	locale = LOCALE_MK;
	else
	if(str == "nl" || str == "dutch")	locale = LOCALE_NL;
	else
	if(str == "pl" || str == "polish")	locale = LOCALE_PL;
	else
	if(str == "pt" || str == "portuguese")	locale = LOCALE_PT;
	else
	if(str == "ru" || str == "russian")	locale = LOCALE_RU;
	else
	if(str == "sk" || str == "slovak")	locale = LOCALE_SK;
	else
	if(str == "sl" || str == "slovenian")	locale = LOCALE_SL;
	else
	if(str == "sr" || str == "serbian")	locale = LOCALE_SR;
	else
	if(str == "sv" || str == "swedish")	locale = LOCALE_SV;
	else
	if(str == "tr" || str == "turkish")	locale = LOCALE_TR;

	return domains[domain].open(file);
    }

    bool setDomain(const char* domain)
    {
	std::map<std::string, mofile>::iterator it = domains.find(domain);
	if(it == domains.end())
	    return false;

	current = & (*it).second;
	return true;
    }

    const char* gettext(const std::string & str)
    {
	return gettext(str.c_str());
    }

    const char* gettext(const char* str)
    {
	return stripContext(current ? current->ngettext(str, 0) : str);
    }

    const char* dgettext(const char* domain, const char* str)
    {
	setDomain(domain);
	return gettext(str);
    }

    const char* ngettext(const char* str, const char* plural, size_t n)
    {
	if(current)
	    switch(locale)
	{
	    case LOCALE_AF:
	    case LOCALE_EU:
	    case LOCALE_ID:
	    case LOCALE_LA:
	    case LOCALE_TR:
		return stripContext(current->ngettext(str, 0));
	    case LOCALE_AR:
		return stripContext(current->ngettext(str, (n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5)));
	    case LOCALE_BG:
	    case LOCALE_DA:
	    case LOCALE_DE:
	    case LOCALE_ES:
	    case LOCALE_ET:
	    case LOCALE_FI:
	    case LOCALE_GL:
	    case LOCALE_HE:
	    case LOCALE_IT:
	    case LOCALE_NL:
	    case LOCALE_SV:
		return stripContext(current->ngettext(str, (n != 1)));
	    case LOCALE_SK:
		return stripContext(current->ngettext(str, ((n==1) ? 1 : (n>=2 && n<=4) ? 2 : 0)));
	    case LOCALE_SL:
		return stripContext(current->ngettext(str, (n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3)));
	    case LOCALE_SR:
		return stripContext(current->ngettext(str, (n==1 ? 3 : n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)));
	    case LOCALE_CS:
		return stripContext(current->ngettext(str, ((n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2)));
	    case LOCALE_EL:
	    case LOCALE_FR:
	    case LOCALE_PT:
		return stripContext(current->ngettext(str, (n > 1)));
	    case LOCALE_HR:
	    case LOCALE_RU:
	    case LOCALE_LT:
	    case LOCALE_LV:
		return stripContext(current->ngettext(str, (n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)));
	    case LOCALE_MK:
		return stripContext(current->ngettext(str, (n==1 || n%10==1 ? 0 : 1)));
	    case LOCALE_PL:
		return stripContext(current->ngettext(str, (n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)));
	    default: break;
	}

	return stripContext(n == 1 ? str : plural);
    }

    const char* dngettext(const char* domain, const char* str, const char* plural, size_t num)
    {
	setDomain(domain);
	return ngettext(str, plural, num);
    }
}
