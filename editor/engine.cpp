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
#include <QtEndian>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPixmapCache>
#include <QDomDocument>
#include <QPainter>

#include "program.h"
#include "mapdata.h"
#include "global.h"
#include "engine.h"

QStringList Editor::townNames(void)
{
    const char* towns[] = { "Blackridge", "Pinehurst", "Woodhaven", "Hillstone", "Whiteshield", "Bloodreign", "Dragontooth", "Greywind", "Blackwind", "Portsmith", "Middle Gate", "Tundara", 
	"Vulcania", "Sansobar", "Atlantium", "Baywatch", "Wildabar", "Fountainhead", "Vertigo", "Winterkill", "Nightshadow", "Sandcaster", "Lakeside", "Olympus", 
	"Brindamoor", "Burlock", "Xabran", "Dragadune", "Alamar", "Kalindra", "Blackfang", "Basenji", "Algary", "Sorpigal", "New Dawn", "Erliquin", 
	"Avone", "Big Oak", "Hampshire", "Chandler", "South Mill", "Weed Patch", "Roc Haven", "Avalon", "Antioch", "Brownston", "Weddington", "Whittingham",
	"Westfork", "Hilltop", "Yorksford", "Sherman", "Roscomon", "Elk's Head", "Cathcart", "Viper's Nest", "Pig's Eye", "Blacksford", "Burton", "Blackburn",
	"Lankershire", "Lombard", "Timberhill", "Fenton", "Troy", "Forder Oaks", "Meramec", "Quick Silver", "Westmoor", "Willow", "Sheltemburg", "Corackston" };

    QStringList res;
    for(int it = 0; it < 72; ++it) res << towns[it];
    return res;
}

QString Portrait::transcribe(int port)
{
    const char* ports[] = { "Unknown",
	"Lord Kilburn", "Sir Gallanth", "Ector", "Gwenneth", "Tyro", "Ambrose", "Ruby", "Maximus", "Dimitry",
	"Thundax", "Fineous", "Jojosh", "Crag Hack", "Jezebel", "Jaclyn", "Ergon", "Tsabu", "Atlas",
	"Astra", "Natasha", "Troyan", "Vatawna", "Rebecca", "Gem", "Ariel", "Carlawn", "Luna",
	"Arie", "Alamar", "Vesper", "Crodo", "Barok", "Kastore", "Agar", "Falagar", "Wrathmont",
	"Myra", "Flint", "Dawn", "Halon", "Myrini", "Wilfrey", "Sarakin", "Kalindra", "Mandigal",
	"Zom", "Darlana", "Zam", "Ranloo", "Charity", "Rialdo", "Roxana", "Sandro", "Celia",
	"Roland", "Lord Corlagon", "Sister Eliza", "Archibald", "Lord Halton", "Brother Bax",
	"Solmyr", "Dainwin", "Mog", "Uncle Ivan", "Joseph", "Gallavant", "Elderian", "Ceallach", "Drakonia", "Martine", "Jarkonas",
	"Random Hero" };

    const int count = sizeof(ports) / sizeof(ports[0]);

    return count > port ? QString(ports[port]) : QString(ports[0]);
}

QString Resource::transcribe(int res)
{
    switch(res)
    {
	case Resource::Wood:	return "Wood";
	case Resource::Mercury:	return "Mercury";
	case Resource::Ore:	return "Ore";
	case Resource::Sulfur:	return "Sulfur";
	case Resource::Crystal:	return "Crystal";
	case Resource::Gems:	return "Gems";
	case Resource::Gold:	return "Gold";
	case Resource::Random:	return "Random";

	default: break;
    }

    return "Unknown";
}

inline bool IS_EQUAL_VALS(int A, int B)
{
    return (A & B) == A;
}

QString readStringFromStream(QDataStream & ds, int count = 0)
{
    QString str;
    quint8 byte;

    if(count)
    {
	str.reserve(128);
	bool endLine = false;

	for(int ii = 0; ii < count; ++ii)
	{
	    ds >> byte;
	    if(0 == byte) endLine = true;
	    if(! endLine) str.push_back(byte);
	}
    }
    else
    {
	str.reserve(512);

	while(! ds.atEnd())
	{
	    ds >> byte;
	    if(0 == byte) break;
	    str.push_back(byte);
	}
    }

    return str;
}

H2::File::File()
{
}

H2::File::File(const QString & fn) : QFile(fn)
{
}

quint32 H2::File::readLE16(void)
{
    quint16 res = 0;
    long long int ps = pos() + sizeof(res);

    if(ps <= size())
    {
	read((char*) &res, sizeof(res));
	res = qFromLittleEndian(res);
    }
    else
	qWarning() << "H2::File::readLE16:" << "out of range";

    return res;
}

quint32 H2::File::readLE32(void)
{
    quint32 res = 0;
    long long int ps = pos() + sizeof(res);

    if(ps <= size())
    {
	read((char*) &res, sizeof(res));
	res = qFromLittleEndian(res);
    }
    else
	qWarning() << "H2::File::readLE32:" << "out of range";

    return res;
}

quint32 H2::File::readByte(void)
{
    quint8 res = 0;
    long long int ps = pos() + sizeof(res);

    if(ps <= size())
    {
	read((char*) &res, 1);
    }
    else
	qWarning() << "H2::File::readByte:" << "out of range";

    return res;
}

QString H2::File::readString(size_t sz)
{
    return QString(readBlock(sz));
}

QByteArray H2::File::readBlock(size_t sz, int ps)
{
    if(0 <= ps)
	seek(ps);

    long long int pps = pos() + sizeof(sz);
    if(pps <= size())
	return read(sz);
    else
	qWarning() << "H2::File::readBlock:" << "out of range";

    return NULL;
}

H2::ICNSprite::ICNSprite(const mp2icn_t & icn, const char* buf, quint32 size, const QVector<QRgb> & pals)
    : QImage(icn.width, icn.height, QImage::Format_ARGB32)
{
    const quint8* ptr = (const quint8*) buf;
    const quint8* outOfRange = ptr + size;

    fill(Qt::transparent);

    if(0x20 == icn.type)
	DrawVariant2(ptr, outOfRange, pals);
    else
	DrawVariant1(ptr, outOfRange, pals);
}

void H2::ICNSprite::DrawVariant1(const quint8* ptr, const quint8* outOfRange, const QVector<QRgb> & pals)
{
    int col = 0;
    int posX = 0;
    int posY = 0;

    QRgb shadow = qRgba(0, 0, 0, 0x40);

    while(1)
    {
        // 0x00 - end line
        if(0 == *ptr)
        {
            posY++;
            posX = 0;
            ptr++;
        }
        else
        // 0x7F - count data
        if(0x80 > *ptr)
        {
            col = *ptr;
            ptr++;
            while(col-- && ptr < outOfRange)
            {
                setPixel(posX, posY, pals[*ptr]);
                posX++;
                ptr++;
            }
        }
        else
        // 0x80 - end data
        if(0x80 == *ptr)
        {
            break;
        }
        else
        // 0xBF - skip data
        if(0xC0 > *ptr)
        {
            posX += *ptr - 0x80;
            ptr++;
        }
        else
        // 0xC0 - shadow
        if(0xC0 == *ptr)
        {
            ptr++;
            col = *ptr % 4 ? *ptr % 4 : *(++ptr);
            while(col--){ setPixel(posX, posY, shadow); posX++; }
            ptr++;
        }
        else
        // 0xC1
        if(0xC1 == *ptr)
        {
            ptr++;
            col = *ptr;
            ptr++;
            while(col--){ setPixel(posY, posY, pals[*ptr]); posX++; }
            ptr++;
        }
        else
        {
            col = *ptr - 0xC0;
            ptr++;
            while(col--){ setPixel(posX, posY, pals[*ptr]); posX++; }
            ptr++;
        }

        if(ptr >= outOfRange)
        {
            qWarning() << "H2::ICNSprite:DrawVariant1:" << "parse out of range";
            break;
        }
    }
}

void H2::ICNSprite::DrawVariant2(const quint8* ptr, const quint8* outOfRange, const QVector<QRgb> & pals)
{
    int col = 0;
    int posX = 0;
    int posY = 0;

    while(1)
    {
        // 0x00 - end line
        if(0 == *ptr)
        {
            posY++;
            posX = 0;
            ptr++;
        }
        else
        // 0x7F - count data
        if(0x80 > *ptr)
        {
	    col = *ptr;
            while(col--) { setPixel(posX, posY, pals[1]); posX++; }
            ptr++;
        }
        else
        // 0x80 - end data
        if(0x80 == *ptr)
        {
            break;
        }
        else
        // other - skip data
        {
	    posX += *ptr - 0x80;
            ptr++;
        }

        if(ptr >= outOfRange)
        {
            qWarning() << "H2::ICNSprite:DrawVariant2:" << "parse out of range";
            break;
        }
    }
}


mp2icn_t::mp2icn_t(const char* data)
{
    QByteArray buf(data, sizeOf());
    QDataStream ds(buf);
    ds >> *this;
}

QDataStream & operator>> (QDataStream & ds, mp2icn_t & icn)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> icn.offsetX >> icn.offsetY >>
	icn.width >> icn.height >> icn.type >> icn.offsetData;
    return ds;
}

mp2lev_t::mp2lev_t()
{
    object = 0;
    index = 0;
    uniq = 0;
}

mp2til_t::mp2til_t()
{
    tileSprite = Editor::Rand(16, 19);
    quantity1 = 0;
    quantity2 = 0;
    tileShape = 0;
    objectID = 0;
}

mp2ext_t::mp2ext_t()
{
    indexExt = 0;
    quantity = 0;
}

QDataStream & operator>> (QDataStream & ds, mp2til_t & til)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    til.ext.quantity = 0;
    return ds >> til.tileSprite >>
	til.ext.level1.object >> til.ext.level1.index >>
	til.quantity1 >> til.quantity2 >>
	til.ext.level2.object >> til.ext.level2.index >>
	til.tileShape >> til.objectID >>
	til.ext.indexExt >> til.ext.level1.uniq >> til.ext.level2.uniq;
}

QDataStream & operator>> (QDataStream & ds, mp2ext_t & ext)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> ext.indexExt >> ext.level1.object >> ext.level1.index >>
	ext.quantity >> ext.level2.object >> ext.level2.index >> ext.level1.uniq >> ext.level2.uniq;
    ext.level1.object *= 2;
    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2town_t & cstl)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> cstl.color >> cstl.customBuilding >> cstl.building >> cstl.magicTower >> cstl.customTroops;

    for(int ii = 0; ii < 5; ++ii)
    { ds >> cstl.troopId[ii]; cstl.troopId[ii] += 1; }

    for(int ii = 0; ii < 5; ++ii)
	ds >> cstl.troopCount[ii];

    ds >> cstl.captainPresent >> cstl.customName;

    cstl.name = readStringFromStream(ds, 13);
    ds >> cstl.race >> cstl.isCastle >> cstl.forceTown;

    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2hero_t & hero)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> hero.unknown1 >> hero.customTroops;

    for(int ii = 0; ii < 5; ++ii)
    { ds >> hero.troopId[ii]; hero.troopId[ii] += 1; }

    for(int ii = 0; ii < 5; ++ii)
	ds >> hero.troopCount[ii];

    ds >> hero.customPortrate >> hero.portrateType;

    for(int ii = 0; ii < 3; ++ii)
    {
	ds >> hero.artifacts[ii];
	hero.artifacts[ii] = 0xFF == hero.artifacts[ii] ? Artifact::None : hero.artifacts[ii] + 1;
    }

    ds >> hero.unknown2 >> hero.experience >> hero.customSkills;

    for(int ii = 0; ii < 8; ++ii)
    {
	ds >> hero.skillId[ii];
	hero.skillId[ii] = 0xFF == hero.skillId[ii] ? SkillType::Unknown : hero.skillId[ii] + 1;
    }

    for(int ii = 0; ii < 8; ++ii)
	ds >> hero.skillLevel[ii];

    ds >> hero.unknown3 >> hero.customName;

    hero.name = readStringFromStream(ds, 13);

    ds >> hero.patrol >> hero.patrolSquare;
    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2sign_t & sign)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> sign.id;

    for(int ii = 0; ii < 8; ++ii)
	ds >> sign.zero[ii];

    sign.text = readStringFromStream(ds);
    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2mapevent_t & evnt)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> evnt.id;

    for(int ii = 0; ii < 7; ++ii)
	ds >> evnt.resources[ii];

    ds >> evnt.artifact >> evnt.allowComputer >> evnt.cancelAfterFirstVisit;
    evnt.artifact = 0xFF == evnt.artifact ? Artifact::None : evnt.artifact + 1;

    for(int ii = 0; ii < 10; ++ii)
	ds >> evnt.zero[ii];

    for(int ii = 0; ii < 6; ++ii)
	ds >> evnt.colors[ii];

    evnt.text = readStringFromStream(ds);
    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2dayevent_t & evnt)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> evnt.id;

    for(int ii = 0; ii < 7; ++ii)
	ds >> evnt.resources[ii];

    ds >> evnt.artifact >> evnt.allowComputer >> evnt.dayFirstOccurent >> evnt.subsequentOccurrences;
    evnt.artifact = 0xFF == evnt.artifact ? Artifact::None : evnt.artifact + 1;

    for(int ii = 0; ii < 6; ++ii)
	ds >> evnt.zero[ii];

    for(int ii = 0; ii < 6; ++ii)
	ds >> evnt.colors[ii];

    evnt.text = readStringFromStream(ds);
    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2rumor_t & rumor)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> rumor.id;

    for(int ii = 0; ii < 7; ++ii)
	ds >> rumor.zero[ii];

    rumor.text = readStringFromStream(ds);
    return ds;
}

QDataStream & operator>> (QDataStream & ds, mp2sphinx_t & sphinx)
{
    ds.setByteOrder(QDataStream::LittleEndian);
    ds >> sphinx.id;

    for(int ii = 0; ii < 7; ++ii)
	ds >> sphinx.resources[ii];

    quint8 answersCount = 0;
    ds >> sphinx.artifact >> answersCount;
    sphinx.artifact = 0xFF == sphinx.artifact ? Artifact::None : sphinx.artifact + 1;

    for(int ii = 0; ii < 8; ++ii)
    {
	QString str = readStringFromStream(ds, 13);

	if(! str.isEmpty())
	    sphinx.answers << str;
    }

    sphinx.text = readStringFromStream(ds);
    return ds;
}

bool AGG::File::exists(const QString & str) const
{
    return items.end() != items.find(str);
}

int AGG::File::seekToData(const QString & name)
{
    QMap<QString, Item>::const_iterator it = items.find(name);

    if(items.end() != it)
	seek((*it).offset);
    else
	qCritical() << "AGG::File::seekToData:" << "item" << qPrintable(name) << "not found";

    return items.end() != it ? (*it).size : 0;
}

QByteArray AGG::File::readRawData(const QString & name)
{
    return readBlock(seekToData(name));
}

bool AGG::File::loadFile(const QString & fn)
{
    if(isOpen()) close();
    if(fn.isEmpty()) return false;

    setFileName(fn);
    if(open(QIODevice::ReadOnly))
    {
	qDebug() << "AGG::File::loadFile:" << qPrintable(fn);
	quint16 countItems = readLE16();

	qDebug() << "AGG::File::loadFile:" << "count items:" << countItems;
	const int sizeName = 15;

	for(int it = 0; it < countItems; ++it)
	{
	    int posname = size() - sizeName * (countItems - it);

	    Item & item = items[QString(readBlock(sizeName, posname))];

	    seek(sizeof(countItems) + it * (3 * sizeof(quint32) /* crcItem + offsetItem + sizeItem */)); 

	    item.crc = readLE32();
	    item.offset = readLE32();
	    item.size = readLE32();
	}

	return true;
    }
    else
	qCritical() << "AGG::File::loadFile:" << "Can not read file " << qPrintable(fn);

    return false;
}

QPixmap AGG::File::getImageTIL(const QString & id, int index, QVector<QRgb> & colors)
{
    int blockSize = seekToData(id);

    if(blockSize)
    {
	quint16 tileCount = readLE16();

	if(index < tileCount)
	{
	    quint16 tileWidth = readLE16();
	    quint16 tileHeight = readLE16();
	    QByteArray buf = readBlock(blockSize - 6);

	    QImage image((uchar*) buf.data() + index * tileWidth * tileHeight, tileWidth, tileHeight, QImage::Format_Indexed8);
	    image.setColorTable(colors);

	    return QPixmap::fromImage(image);
	}
	else
	    qCritical() << "AGG::File::getImageTIL:" << "out of range" << index;
    }

    return QPixmap();
}

QPair<QPixmap, QPoint> AGG::File::getImageICN(const QString & id, int index, QVector<QRgb> & colors)
{
    QPair<QPixmap, QPoint> result;
    int blockSize = seekToData(id);

    if(blockSize)
    {
	quint16 icnCount = readLE16();

	if(index < icnCount)
	{
	    quint32 sizeData = readLE32();
	    QByteArray buf = readBlock(blockSize - 6);
	    mp2icn_t header(buf.data() + index * mp2icn_t::sizeOf());

	    if(index + 1 < icnCount)
	    {
		mp2icn_t headerNext(buf.data() + (index + 1) * mp2icn_t::sizeOf());
		sizeData = headerNext.offsetData - header.offsetData;
	    }
	    else
		sizeData = sizeData - header.offsetData;

	    H2::ICNSprite image(header, buf.data() + header.offsetData, sizeData, colors);

	    result.first = QPixmap::fromImage(image);
	    result.second = QPoint(header.offsetX, header.offsetY);
	}
	else
	    qCritical() << "AGG::File::getImageICN:" << "out of range" << index;
    }

    return result;
}

QString AGG::Spool::dirName(void) const
{
    return first.isReadable() ?
	QDir::toNativeSeparators(QFileInfo(first.fileName()).absolutePath()) : NULL;
}

bool AGG::Spool::setData(const QString & file)
{
    if(first.loadFile(file))
    {
	QFileInfo fileInfo(file);
	QStringList list = fileInfo.absoluteDir().entryList(QStringList() << "heroes2x.agg", QDir::Files | QDir::Readable);

	if(list.size())
	    second.loadFile(QDir::toNativeSeparators(fileInfo.absolutePath() + QDir::separator() + list.front()));

	// load palette
	QByteArray array = first.readRawData("KB.PAL");

	if(array.size())
	{
	    qint8 r, g, b;
	    const quint32 palSize = array.size() / 3;
	    colors.reserve(palSize);

	    for(quint32 num = 0; num < palSize; ++num)
	    {
		r = array[num * 3];
		g = array[num * 3 + 1];
		b = array[num * 3 + 2];

		colors.push_back(qRgb(r << 2, g << 2, b << 2));
	    }

	    qDebug() << "AGG::Spool:" << "loaded palette: " << colors.size();

	    return true;
	}
	else
	    qCritical() << "AGG::Spool:" << "palette not found";
    }

    return false;
}

bool AGG::Spool::isHeroes2XMode(void) const
{
    return second.isReadable();
}

QPixmap AGG::Spool::getImageTIL(const QString & til, int index)
{
    QString key = til + QString::number(index);
    QPixmap result;

    if(! QPixmapCache::find(key, & result))
    {
	result = second.isReadable() && second.exists(til) ?
		second.getImageTIL(til, index, colors) : first.getImageTIL(til, index, colors);
	QPixmapCache::insert(key, result);
    }

    return result;
}

void AGG::Spool::fixAGGImagesBugs(const QString & icn, int index, QPair<QPixmap, QPoint> & result)
{
    // fix images
    switch(EditorTheme::mapICN(icn))
    {
	case ICN::MINIHERO:
	    // fix orange color
	    if(index == 34)
	    {
		QPair<QPixmap, QPoint> goodImage = getImageICN(icn, 33);
    		QPainter paint(& result.first);
        	paint.drawPixmap(QRect(0,0,23,13), goodImage.first, QRect(0,0,23,13));
	    }
	    break;

	default: break;		
    }
}

QPair<QPixmap, QPoint> AGG::Spool::getImageICN(const QString & icn, int index)
{
    QString key = icn + QString::number(index);
    QPair<QPixmap, QPoint> result;

    if(0 <= index)
    {
	if(! QPixmapCache::find(key, & result.first))
	{
	    result = second.isReadable() && second.exists(icn) ?
		second.getImageICN(icn, index, colors) : first.getImageICN(icn, index, colors);

	    fixAGGImagesBugs(icn, index, result);

	    QPixmapCache::insert(key, result.first);
	    icnOffsetCache[key] = result.second;
	}
	else
	{
	    result.second = icnOffsetCache[key];
	}
    }

    return result;
}

QPixmap AGG::Spool::getImage(const CompositeObject & obj, const QSize & tileSize)
{
    const QString key = obj.icn + obj.name;
    QPixmap result;

    if(! QPixmapCache::find(key, & result))
    {
	int width = obj.size.width() * tileSize.width();
	int height = obj.size.height() * tileSize.height();

	result = QPixmap(width, height);
	result.fill(Qt::transparent);
	QPainter paint(& result);

	for(QVector<CompositeSprite>::const_iterator
	    it = obj.begin(); it != obj.end(); ++it)
	{
	    const QString & icnStr = ICN::transcribe((*it).spriteICN);
	    QPoint offset((*it).spritePos.x() * tileSize.width(), (*it).spritePos.y() * tileSize.height());
	    QPair<QPixmap, QPoint> sprite = getImageICN(icnStr, (*it).spriteIndex);
	    paint.drawPixmap(offset + sprite.second, sprite.first);

	    if((*it).spriteAnimation)
	    {
		sprite = getImageICN(icnStr, (*it).spriteIndex + 1);
		paint.drawPixmap(offset + sprite.second, sprite.first);
	    }
	}

	QPixmapCache::insert(key, result);
    }

    return result;
}

float Editor::RandF(float min, float max)
{
    if(min > max) qSwap(min, max);
    return min + RandF(max - min);
}

float Editor::RandF(float max)
{
    return qrand() * (max) / static_cast<float>(RAND_MAX);
}

quint32 Editor::Rand(quint32 min, quint32 max)
{
    if(min > max) qSwap(min, max);
    return min + Rand(max - min);
}

quint32 Editor::Rand(quint32 max)
{
    return static_cast<quint32>((max + 1) * (qrand() / (RAND_MAX + 1.0)));
}

QPixmap Editor::pixmapBorder(const QSize & size, const QColor & fillCol, const QColor & bordCol)
{
    QPixmap result(size);
    result.fill(fillCol);

    QPainter paint(& result);
    paint.setPen(QPen(bordCol, 1));
    paint.setBrush(QBrush(QColor(0, 0, 0, 0)));
    paint.drawRect(0, 0, size.width() - 1, size.height() - 1);

    return result;
}

QPixmap Editor::pixmapBorderPassable(int passable)
{
    QPixmap result;
    QString key = "passable_" + QString::number(passable);

    if(! QPixmapCache::find(key, & result))
    {
	QColor redColor(255, 0, 0);
	QColor greenColor(0, 255, 0);
	QSize size = EditorTheme::tileSize() - QSize(2, 2);

	if(Direction::Unknown == passable || Direction::Center == passable)
	    result = pixmapBorder(size, Qt::transparent, redColor);
	else
	if(IS_EQUAL_VALS(Direction::All, passable))
	    result = pixmapBorder(size, Qt::transparent, greenColor);
	else
	{
	    result = QPixmap(size);
	    result.fill(Qt::transparent);

	    int cw = (size.width() - 3) / 3;
	    int ch = (size.height() - 3) / 3;

	    QPainter paint(& result);
	    paint.setBrush(QBrush(QColor(0, 0, 0, 0)));

	    for(int xx = 1; xx < size.width() - 2; ++xx)
	    {
		if(xx < cw)
		{
		    paint.setPen(QPen((passable & Direction::TopLeft ? greenColor : redColor), 1));
		    paint.drawPoint(xx, 1);
		    paint.setPen(QPen((passable & Direction::BottomLeft ? greenColor : redColor), 1));
		    paint.drawPoint(xx, size.height() - 2);
		}
		else
		if(xx < 2 * cw)
		{
		    paint.setPen(QPen((passable & Direction::Top ? greenColor : redColor), 1));
		    paint.drawPoint(xx, 1);
		    paint.setPen(QPen((passable & Direction::Bottom ? greenColor : redColor), 1));
		    paint.drawPoint(xx, size.height() - 2);
		}
		else
		{
		    paint.setPen(QPen((passable & Direction::TopRight ? greenColor : redColor), 1));
		    paint.drawPoint(xx, 1);
		    paint.setPen(QPen((passable & Direction::BottomRight ? greenColor : redColor), 1));
		    paint.drawPoint(xx, size.height() - 2);
		}
	    }

	    for(int yy = 1; yy < size.height() - 2; ++yy)
	    {
		if(yy < ch)
		{
		    paint.setPen(QPen((passable & Direction::TopLeft ? greenColor : redColor), 1));
		    paint.drawPoint(1, yy);
		    paint.setPen(QPen((passable & Direction::TopRight ? greenColor : redColor), 1));
		    paint.drawPoint(size.width() - 2, yy);
		}
		else
		if(yy < 2 * ch)
		{
		    paint.setPen(QPen((passable & Direction::Left ? greenColor : redColor), 1));
		    paint.drawPoint(1, yy);
		    paint.setPen(QPen((passable & Direction::Right ? greenColor : redColor), 1));
		    paint.drawPoint(size.width() - 2, yy);
		}
		else
		{
	    	    paint.setPen(QPen((passable & Direction::BottomLeft ? greenColor : redColor), 1));
		    paint.drawPoint(1, yy);
	    	    paint.setPen(QPen((passable & Direction::BottomRight ? greenColor : redColor), 1));
		    paint.drawPoint(size.width() - 2, yy);
		}
	    }
	}

	QPixmapCache::insert(key, result);
    }

    return result;
}

Editor::MyXML::MyXML(const QString & xml, const QString & root, bool debug)
{
    QDomDocument dom;
    QFile file(xml);

    if(file.open(QIODevice::ReadOnly))
    {
        QString errorStr;
        int errorLine;
        int errorColumn;

	if(! dom.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
    	    qDebug() << "MyXML:" << xml << errorStr << errorLine << errorColumn;
    }
    else
	qDebug() << "MyXML:" << "error open:" << xml;

    file.close();

    static_cast<QDomElement &>(*this) = dom.documentElement();

    if(tagName() != root)
    {
	if(debug)
	    qDebug() << "MyXML:" << xml << "unknown tag:" << tagName();
	clear();
    }
}

Editor::MyObjectsXML::MyObjectsXML(const QString & xml, bool debug)
{
    Editor::MyXML templateObjects(QDir::toNativeSeparators(QFileInfo(xml).absolutePath() + QDir::separator() + "template.xml"), "template");
    Editor::MyXML objectsElem(xml, "objects", debug);

    if(! objectsElem.isNull())
    {
	QString icn = objectsElem.hasAttribute("icn") ? objectsElem.attribute("icn") : NULL;
	int cid = objectsElem.hasAttribute("cid") ? objectsElem.attribute("cid").toInt(NULL, 0) : MapObj::None;
    
	// parse element: object
        QDomNodeList objectsList = objectsElem.elementsByTagName("object");

        for(int pos = 0; pos < objectsList.size(); ++pos)
        {
            QDomElement objElem = objectsList.item(pos).toElement();

            if(! objElem.hasAttribute("cid")) objElem.setAttribute("cid", cid);
            if(! objElem.hasAttribute("icn")) objElem.setAttribute("icn", icn.isNull() ? "unknown" : icn);

	    push_back(objElem);
        }

        if(! templateObjects.isNull())
	{
    	    // parse element: template
    	    objectsList = objectsElem.elementsByTagName("template");

    	    for(int pos = 0; pos < objectsList.size(); ++pos)
    	    {
        	QDomElement tmplElem(objectsList.item(pos).toElement());

        	if(tmplElem.hasAttribute("section"))
        	{
            	    QDomElement objElem = templateObjects.firstChildElement(tmplElem.attribute("section")).cloneNode(true).toElement();

            	    if(objElem.isNull())
            	    {
			if(debug)
                	    qDebug() << "Editor::MyObjectsXML:" << "unknown xml section:" << tmplElem.attribute("section") << "file:" << xml;
                	continue;
            	    }

		    if(tmplElem.hasAttribute("hide")) objElem.setAttribute("hide", tmplElem.attribute("hide"));

		    if(tmplElem.hasAttribute("icn")) objElem.setAttribute("icn", tmplElem.attribute("icn"));
        	    else
		    if(!icn.isNull()) objElem.setAttribute("icn", icn);

		    if(tmplElem.hasAttribute("cid")) objElem.setAttribute("cid", tmplElem.attribute("cid").toInt(NULL, 0));
        	    else
		    if(cid != MapObj::None) objElem.setAttribute("cid", cid);

            	    objElem.setAttribute("name", tmplElem.hasAttribute("name") ? tmplElem.attribute("name") : objElem.tagName());
            	    objElem.setTagName("object");
		    
		    // fix index offset
        	    int offsetIndex = tmplElem.attribute("index").toInt();
    		    QDomNodeList spritesList = objElem.elementsByTagName("sprite");

    		    for(int pos2 = 0; pos2 < spritesList.size(); ++pos2)
    		    {
        		QDomElement spriteElem = spritesList.item(pos2).toElement();
        		int startIndex = spriteElem.hasAttribute("index") ? spriteElem.attribute("index").toInt(NULL, 0) : 0;
			bool skipoff = spriteElem.hasAttribute("skipoffset") && 0 != spriteElem.attribute("skipoffset").toInt(NULL, 0);
			spriteElem.setAttribute("index", startIndex + (skipoff ? 0 : offsetIndex));
		    }

		    push_back(objElem);
		}
	    }
	}
    }
}

int H2::MP2ICN(int type, bool warn)
{
    switch(type & 0xFC)
    {
	// artifact
	case 0x2C: return ICN::OBJNARTI;
	// monster
	case 0x30: return ICN::MONS32;
	// castle flags
	case 0x38: return ICN::FLAG32;
	// heroes
	case 0x54: return ICN::MINIHERO;
	// relief: snow
	case 0x58: return ICN::MTNSNOW;
	// relief: swamp
	case 0x5C: return ICN::MTNSWMP;
	// relief: lava
	case 0x60: return ICN::MTNLAVA;
	// relief: desert
	case 0x64: return ICN::MTNDSRT;
	// relief: dirt
	case 0x68: return ICN::MTNDIRT;
	// relief: others
	case 0x6C: return ICN::MTNMULT;
	// mines
	case 0x74: return ICN::EXTRAOVR;
	// road
	case 0x78: return ICN::ROAD;
	// relief: crck
	case 0x7C: return ICN::MTNCRCK;
	// relief: gras
	case 0x80: return ICN::MTNGRAS;
	// trees jungle
	case 0x84: return ICN::TREJNGL;
	// trees evil
	case 0x88: return ICN::TREEVIL;
	// castle and tower
	case 0x8C: return ICN::OBJNTOWN;
	// castle lands
	case 0x90: return ICN::OBJNTWBA;
	// castle shadow
	case 0x94: return ICN::OBJNTWSH;
	// random castle
	case 0x98: return ICN::OBJNTWRD;
	// water object
	case 0xA0: return ICN::OBJNWAT2;
	// object other
	case 0xA4: return ICN::OBJNMUL2;
	// trees snow
	case 0xA8: return ICN::TRESNOW;
	// trees trefir
	case 0xAC: return ICN::TREFIR;
	// trees
	case 0xB0: return ICN::TREFALL;
	// river
	case 0xB4: return ICN::STREAM;
	// resource
	case 0xB8: return ICN::OBJNRSRC;
	// gras object
	case 0xC0: return ICN::OBJNGRA2;
	// trees tredeci
	case 0xC4: return ICN::TREDECI;
	// sea object
	case 0xC8: return ICN::OBJNWATR;
        // vegetation gras                                            
        case 0xCC: return ICN::OBJNGRAS;
	// object on snow                                             
	case 0xD0: return ICN::OBJNSNOW;
        // object on swamp                                            
        case 0xD4: return ICN::OBJNSWMP;
	// object on lava                                             
	case 0xD8: return ICN::OBJNLAVA;
        // object on desert                                           
        case 0xDC: return ICN::OBJNDSRT;
        // object on dirt                                             
	case 0xE0: return ICN::OBJNDIRT;
	// object on crck
	case 0xE4: return ICN::OBJNCRCK;
	// object on lava
	case 0xE8: return ICN::OBJNLAV3;
	// object on earth
	case 0xEC: return ICN::OBJNMULT;
	//  object on lava                                            
	case 0xF0: return ICN::OBJNLAV2;
	// extra objects for loyalty version
	case 0xF4: return ICN::X_LOC1;
	// extra objects for loyalty version
	case 0xF8: return ICN::X_LOC2;
	// extra objects for loyalty version
	case 0xFC: return ICN::X_LOC3;
	// unknown
	default: if(warn) qWarning() << "H2::MP2ICN: unknown object:" << type; break;
    }

    return ICN::UNKNOWN;
}

QString ICN::transcribe(int type)
{
    switch(type)
    {
	case ICN::OBJNARTI: return "OBJNARTI.ICN";
	case ICN::MONS32: return "MONS32.ICN";
	case ICN::FLAG32: return "FLAG32.ICN";
	case ICN::MINIHERO: return "MINIHERO.ICN";
	case ICN::MTNSNOW: return "MTNSNOW.ICN";
	case ICN::MTNSWMP: return "MTNSWMP.ICN";
	case ICN::MTNLAVA: return "MTNLAVA.ICN";
	case ICN::MTNDSRT: return "MTNDSRT.ICN";
	case ICN::MTNDIRT: return "MTNDIRT.ICN";
	case ICN::MTNMULT: return "MTNMULT.ICN";
	case ICN::EXTRAOVR: return "EXTRAOVR.ICN";
	case ICN::ROAD: return "ROAD.ICN";
	case ICN::MTNCRCK: return "MTNCRCK.ICN";
	case ICN::MTNGRAS: return "MTNGRAS.ICN";
	case ICN::TREJNGL: return "TREJNGL.ICN";
	case ICN::TREEVIL: return "TREEVIL.ICN";
	case ICN::OBJNTOWN: return "OBJNTOWN.ICN";
	case ICN::OBJNTWBA: return "OBJNTWBA.ICN";
	case ICN::OBJNTWSH: return "OBJNTWSH.ICN";
	case ICN::OBJNTWRD: return "OBJNTWRD.ICN";
	case ICN::OBJNWAT2: return "OBJNWAT2.ICN";
	case ICN::OBJNMUL2: return "OBJNMUL2.ICN";
	case ICN::TRESNOW: return "TRESNOW.ICN";
	case ICN::TREFIR: return "TREFIR.ICN";
	case ICN::TREFALL: return "TREFALL.ICN";
	case ICN::STREAM: return "STREAM.ICN";
	case ICN::OBJNRSRC: return "OBJNRSRC.ICN";
	case ICN::OBJNGRA2: return "OBJNGRA2.ICN";
	case ICN::TREDECI: return "TREDECI.ICN";
	case ICN::OBJNWATR: return "OBJNWATR.ICN";
        case ICN::OBJNGRAS: return "OBJNGRAS.ICN";
	case ICN::OBJNSNOW: return "OBJNSNOW.ICN";
        case ICN::OBJNSWMP: return "OBJNSWMP.ICN";
	case ICN::OBJNLAVA: return "OBJNLAVA.ICN";
        case ICN::OBJNDSRT: return "OBJNDSRT.ICN";
	case ICN::OBJNDIRT: return "OBJNDIRT.ICN";
	case ICN::OBJNCRCK: return "OBJNCRCK.ICN";
	case ICN::OBJNLAV3: return "OBJNLAV3.ICN";
	case ICN::OBJNMULT: return "OBJNMULT.ICN";
	case ICN::OBJNLAV2: return "OBJNLAV2.ICN";
	case ICN::X_LOC1: return "X_LOC1.ICN";
	case ICN::X_LOC2: return "X_LOC2.ICN";
	case ICN::X_LOC3: return "X_LOC3.ICN";
	// unknown
	default: break;
    }

    return NULL;
}

int H2::isAnimationICN(int spriteClass, int spriteIndex, int ticket)
{
    switch(spriteClass)
    {
	case ICN::OBJNLAVA:
	    switch(spriteIndex)
	    {
		// shadow of lava
		case 0x4E: case 0x58: case 0x62:
		    return spriteIndex + (ticket % 9) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNLAV2:
	    switch(spriteIndex)
	    {
		// middle volcano
		case 0x00:
		// shadow
		case 0x07: case 0x0E:
		// lava
		case 0x15:
		    return spriteIndex + (ticket % 6) + 1;
		// small volcano
		// shadow
		case 0x21: case 0x2C:
		// lava
		case 0x37: case 0x43:
		    return spriteIndex + (ticket % 10) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNLAV3:
	    // big volcano
	    switch(spriteIndex)
	    {
		// smoke
		case 0x00: case 0x0F: case 0x1E: case 0x2D: case 0x3C: case 0x4B: case 0x5A: case 0x69: case 0x87: case 0x96: case 0xA5:
		// shadow
		case 0x78: case 0xB4: case 0xC3: case 0xD2: case 0xE1:
		    return spriteIndex + (ticket % 14) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNMUL2:
	    switch(spriteIndex)
	    {
		// lighthouse
		case 0x3D:
		    return spriteIndex + (ticket % 9) + 1;
		// alchemytower
		case 0x1B:
		// watermill
		case 0x53: case 0x5A: case 0x62: case 0x69:
		// fire in wagoncamp
		case 0x81:
		// smoke smithy (2 chimney)
		case 0xA6:
		// smoke smithy (1 chimney)
		case 0xAD:
		// shadow smoke
		case 0xB4:
		    return spriteIndex + (ticket % 6) + 1;
		// magic garden
		case 0xBE:
		    return spriteIndex + (ticket % 6) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNSNOW:
	    switch(spriteIndex)
	    {
		// firecamp
		case 0x04:
		// alchemytower
		case 0x97:
		// watermill
		case 0xA2: case 0xA9: case 0xB1: case 0xB8:
		    return spriteIndex + (ticket % 6) + 1;
		// mill
		case 0x60: case 0x64: case 0x68: case 0x6C: case 0x70: case 0x74: case 0x78: case 0x7C: case 0x80: case 0x84:
		    return spriteIndex + (ticket % 3) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNSWMP:
	    switch(spriteIndex)
	    {
		// shadow
		case 0x00: case 0x0E: case 0x2B:
		// smoke
		case 0x07: case 0x22: case 0x33:
		// light in window
		case 0x16: case 0x3A: case 0x43: case 0x4A:
		    return spriteIndex + (ticket % 6) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNDSRT:
	    switch(spriteIndex)
	    {
		// campfire
		case 0x36: case 0x3D:
		    return spriteIndex + (ticket % 6) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNGRA2:
	    switch(spriteIndex)
	    {
		// mill
		case 0x17: case 0x1B: case 0x1F: case 0x23: case 0x27: case 0x2B: case 0x2F: case 0x33: case 0x37: case 0x3B:
		    return spriteIndex + (ticket % 3) + 1;
		// smoke from chimney
		case 0x3F: case 0x46: case 0x4D:
		// archerhouse
		case 0x54:
		// smoke from chimney
		case 0x5D: case 0x64:
		// shadow smoke
		case 0x6B:
		// peasanthunt
		case 0x72:
		    return spriteIndex + (ticket % 6) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNCRCK:
	    switch(spriteIndex)
	    {
		// pool of oil
		case 0x50: case 0x5B: case 0x66: case 0x71: case 0x7C: case 0x89: case 0x94: case 0x9F: case 0xAA:
		// smoke from chimney
		case 0xBE:
		// shadow smoke
		case 0xCA:
		    return spriteIndex + (ticket % 10) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNDIRT:
	    switch(spriteIndex)
	    {
		// mill
		case 0x99: case 0x9D: case 0xA1: case 0xA5: case 0xA9: case 0xAD: case 0xB1: case 0xB5: case 0xB9: case 0xBD:
		    return spriteIndex + (ticket % 3) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNMULT:
	    switch(spriteIndex)
	    {
		// smoke
		case 0x05:
		// shadow
		case 0x0F: case 0x19:
		    return spriteIndex + (ticket % 9) + 1;
		// smoke
		case 0x24:
		// shadow
		case 0x2D:
		    return spriteIndex + (ticket % 8) + 1;
		// smoke
		case 0x5A:
		// shadow
		case 0x61: case 0x68: case 0x7C:
		// campfire
		case 0x83:
		    return spriteIndex + (ticket % 6) + 1;
		default: break;
	    }
	    break;

	case ICN::OBJNWATR:
	    switch(spriteIndex)
	    {
		// buttle
		case 0x00:
		    return spriteIndex + (ticket % 11) + 1;
		// shadow
		case 0x0C:
		// chest
		case 0x13:
		// shadow
		case 0x26:
		// flotsam
		case 0x2D:
		// unkn
		case 0x37:
		// boat
		case 0x3E:
		// waves
		case 0x45:
		// seaweed
		case 0x4C: case 0x53: case 0x5A: case 0x61: case 0x68:
		// sailor-man
		case 0x6F:
		// shadow
		case 0xBC:
		// buoy
		case 0xC3:
		// broken ship (right)
		case 0xE2: case 0xE9: case 0xF1: case 0xF8:
		    return spriteIndex + (ticket % 6) + 1;
		// seagull on stones
		case 0x76: case 0x86: case 0x96: case 0xA6:
		    return spriteIndex + (ticket % 15) + 1;
		// whirlpool
		case 0xCA: case 0xCE: case 0xD2: case 0xD6: case 0xDA: case 0xDE:
		    return spriteIndex + (ticket % 3) + 1;
		default: break;

	    }
	    break;

	case ICN::OBJNWAT2:
	    switch(spriteIndex)
	    {
		// sail broken ship (left)
		case 0x03: case 0x0C:
		    return spriteIndex + (ticket % 6) + 1;
		default: break;
	    }
	    break;

	case ICN::X_LOC1:
	    switch(spriteIndex)
	    {
		// alchemist tower
		case 0x04: case 0x0D: case 0x16:
		// arena
		case 0x1F: case 0x28: case 0x32: case 0x3B:
		// earth altar
		case 0x55: case 0x5E: case 0x67:
		    return spriteIndex + (ticket % 8) + 1;
		default: break;
	    }
	    break;

	case ICN::X_LOC2:
	    switch(spriteIndex)
	    {
		// mermaid
		case 0x0A: case 0x13: case 0x1C: case 0x25: 
		// sirens
		case 0x2F: case 0x38: case 0x41: case 0x4A: case 0x53: case 0x5C: case 0x66:
		    return spriteIndex + (ticket % 8) + 1;
		default: break;
	    }
	    break;

	case ICN::X_LOC3:
	    switch(spriteIndex)
	    {
		// hut magi
		case 0x00: case 0x0A: case 0x14:
		// eye magi
		case 0x20: case 0x29: case 0x32:
		    return spriteIndex + (ticket % 8) + 1;
		// barrier
		case 0x3C: case 0x42: case 0x48: case 0x4E: case 0x54: case 0x5A: case 0x60: case 0x66:
		    return spriteIndex + (ticket % 4) + 1;
		default: break;
	    }

	default: break;
    }

    return 0;
}

struct SpriteInfo
{
    int		oid;
    int		level;
    int		passable;

    SpriteInfo() : oid(MapObj::None), level(SpriteLevel::Unknown), passable(Direction::Unknown) {}

    SpriteInfo(int id, int lv, int ps) : oid(id), level(lv), passable(ps)
    {
	if(level == SpriteLevel::Top) passable = Direction::All;
	else
	if(level == SpriteLevel::Action) oid |= MapObj::IsAction;
    }
};

/*Themes section */
namespace EditorTheme
{
    AGG::Spool			aggSpool;
    QString			themeName("unknown");
    QSize			themeTile(0, 0);
    QMap<int, SpriteInfo>	mapSpriteInfoCache;
    QMap<QString, int>		mapICNs;

    QPair<int, int>		groundBoundariesFixBase(const AroundGrounds & around, int ground, int groundMarker, int groundAND, int groundNOT);

    const SpriteInfo*		findCacheSprite(int icn, int index)
    {
	int key = (icn << 16) | (0x0000FFFF & index);
	QMap<int,SpriteInfo>::const_iterator it = mapSpriteInfoCache.find(key);
	return it != mapSpriteInfoCache.end() ? & (*it) : NULL;
    }

    QString			checkICN(const QString & str)
    {
	QString res = str.toUpper();
	if(0 > res.lastIndexOf(".ICN")) res.append(".ICN");
	return res;
    }

    void			loadCacheSprites(void)
    {
	mapSpriteInfoCache.clear();
	QStringList files = resourceFiles("objects", "*.xml");
	for(QStringList::const_iterator
	    it = files.begin(); it != files.end(); ++it)
	{
	    Editor::MyObjectsXML objectsElem(*it, false);

	    for(QList<QDomElement>::const_iterator
		it = objectsElem.begin(); it != objectsElem.end(); ++it)
	    {
		int icn0 = mapICNs[checkICN((*it).attribute("icn"))];
		int gcid = (*it).attribute("cid").toInt(NULL, 0);

    		QDomNodeList spritesList = (*it).elementsByTagName("sprite");

    		for(int pos = 0; pos < spritesList.size(); ++pos)
    		{
        	    QDomElement spriteElem = spritesList.item(pos).toElement();
		    int index = spriteElem.attribute("index").toInt();
		    QString level = spriteElem.attribute("level");
		    int passable = spriteElem.attribute("passable").toInt(NULL, 0);
		    int icn = spriteElem.hasAttribute("icn") ? mapICNs[checkICN(spriteElem.attribute("icn"))] : icn0;
		    int key = (icn << 16) | (0x0000FFFF & index);
		    int cid = spriteElem.hasAttribute("cid") ? spriteElem.attribute("cid").toInt(NULL, 0) : gcid;
		    mapSpriteInfoCache[key] = SpriteInfo(cid, SpriteLevel::fromString(level), passable);
		}
	    }
	}
    }
}

int EditorTheme::mapICN(const QString & str)
{
    return mapICNs[str];
}

bool EditorTheme::load(const QString & data)
{
    for(int ii = 0; ii < 0xFF; ++ii)
    {
	if(ICN::UNKNOWN != H2::MP2ICN(ii, false))
	    mapICNs[ICN::transcribe(ii)] = ii;
    }

    if(aggSpool.setData(data))
    {
	themeName = "agg";
	themeTile = QSize(32, 32);

	loadCacheSprites();

	return true;
    }

    return false;
}

QString EditorTheme::resourceFile(const QString & dir, const QString & file)
{
    return Resource::FindFile(QDir::toNativeSeparators(QString("themes") + QDir::separator() + themeName + QDir::separator() + dir), file);
}

QStringList EditorTheme::resourceFiles(const QString & dir, const QString & file)
{
    return Resource::FindFiles(QDir::toNativeSeparators(QString("themes") + QDir::separator() + themeName + QDir::separator() + dir), file);
}

int EditorTheme::getSpriteID(int icn, int index)
{
    const SpriteInfo* spriteInfo = findCacheSprite(icn, index);
    return spriteInfo && spriteInfo->level != SpriteLevel::Top ? spriteInfo->oid : MapObj::None; /* skip top level, shadow and other */
}

int EditorTheme::getSpriteLevel(int icn, int index)
{
    const SpriteInfo* spriteInfo = findCacheSprite(icn, index);
    return spriteInfo ? spriteInfo->level : SpriteLevel::Unknown;
}

int EditorTheme::getSpritePassable(int icn, int index)
{
    const SpriteInfo* spriteInfo = findCacheSprite(icn, index);
    return spriteInfo ? spriteInfo->passable : Direction::Unknown;
}

QPixmap EditorTheme::getImageTIL(const QString & til, int index)
{
    return aggSpool.getImageTIL(til, index);
}

QPair<QPixmap, QPoint> EditorTheme::getImageICN(const QString & icn, int index)
{
    return aggSpool.getImageICN(icn, index);
}

QPair<QPixmap, QPoint> EditorTheme::getImageICN(int icn, int index)
{
    return aggSpool.getImageICN(ICN::transcribe(icn), index);
}

QPixmap EditorTheme::getImage(const CompositeObject & obj)
{
    return aggSpool.getImage(obj, themeTile);
}

const QSize & EditorTheme::tileSize(void)
{
    return themeTile;
}

int EditorTheme::startFilledTile(int ground)
{
    // 30%
    if(0 == Editor::Rand(6))
        return startFilledOriginalTile(ground);

    int res = 0;

    switch(ground)
    {
        case Ground::Desert:      res = 300; break;
        case Ground::Snow:        res = 130; break;
        case Ground::Swamp:       res = 184; break;
        case Ground::Wasteland:   res = 399; break;
        case Ground::Beach:       res = 415; break;
        case Ground::Lava:        res = 246; break;
        case Ground::Dirt:        res = 337; break;
        case Ground::Grass:       res = 68;  break;
        case Ground::Water:       res = 16;  break;
        default: qCritical() << "EditorTheme::startFilledTile:" << "unknown ground"; break;
    }

    return res + Editor::Rand(7);
}

int EditorTheme::startFilledOriginalTile(int ground)
{
    int res = 0;
    int count = 8;

    switch(ground)
    {
        case Ground::Desert:      res = 308; count = 13; break;
        case Ground::Snow:        res = 138; break;
        case Ground::Swamp:       res = 192; count = 16; break;
        case Ground::Wasteland:   res = 407; break;
        case Ground::Beach:       res = 423; count = 9; break;
        case Ground::Lava:        res = 254; break;
        case Ground::Dirt:        res = 345; count = 16; break;
        case Ground::Grass:       res = 76; count = 16; break;
        case Ground::Water:       res = 24; count = 6; break;
        default: qCritical() << "EditorTheme::startFilledOriginalTile:" << "unknown ground"; break;
    }

    return res + Editor::Rand(count - 1);
}

int EditorTheme::ground(int index)
{
    // list grounds from GROUND32.TIL
    if(30 > index)
        return Ground::Water;
    else
    if(92 > index)
        return Ground::Grass;
    else
    if(146 > index)
        return Ground::Snow;
    else
    if(208 > index)
        return Ground::Swamp;
    else
    if(262 > index)
        return Ground::Lava;
    else
    if(321 > index)
        return Ground::Desert;
    else
    if(361 > index)
        return Ground::Dirt;
    else
    if(415 > index)
        return Ground::Wasteland;
    else
    if(432 > index)
        return Ground::Beach;

    return Ground::Unknown;
}

int EditorTheme::startGroundTile(int ground)
{
    int res = 0;

    // from GROUND32.TIL
    switch(ground)
    {
        case Ground::Desert:    return 262;
        case Ground::Snow:      return 92;
        case Ground::Swamp:     return 146;
        case Ground::Wasteland: return 361;
        case Ground::Beach:     return 415;
        case Ground::Lava:      return 208;
        case Ground::Dirt:      return 321;
        case Ground::Grass:     return 30;
        case Ground::Water:     return 0;
        default: break;
    }

    return res;
}

int EditorTheme::startGroundOriginalTile(int ground)
{
    return startGroundTile(ground) + Editor::Rand(3);
}

AroundGrounds::AroundGrounds(const MapTiles & tiles, const QPoint & center) : QVector<int>(9, Ground::Unknown)
{
    QVector<int> & v = *this;
    const MapTile* tile = NULL;

    tile = tiles.tileFromDirectionConst(center, Direction::TopLeft);
    if(tile) v[0] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::Top);
    if(tile) v[1] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::TopRight);
    if(tile) v[2] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::Right);
    if(tile) v[3] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::BottomRight);
    if(tile) v[4] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::Bottom);
    if(tile) v[5] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::BottomLeft);
    if(tile) v[6] = tile->groundType();

    tile = tiles.tileFromDirectionConst(center, Direction::Left);
    if(tile) v[7] = tile->groundType();

    tile = tiles.tileConst(center);
    if(tile) v[8] = tile->groundType();
}

int AroundGrounds::operator() (void) const
{
    int res = 0;

    for(QVector<int>::const_iterator
        it = begin(); it != end(); ++it)
        res |= *it;

    return res;
}

int AroundGrounds::groundsDirects(int directs) const
{
    int res = Ground::Unknown;
    const QVector<int> & v = *this;

    if(Direction::TopLeft & directs)     res |= v[0];
    if(Direction::Top & directs)         res |= v[1];
    if(Direction::TopRight & directs)    res |= v[2];
    if(Direction::Right & directs)       res |= v[3];
    if(Direction::BottomRight & directs) res |= v[4];
    if(Direction::Bottom & directs)      res |= v[5];
    if(Direction::BottomLeft & directs)  res |= v[6];
    if(Direction::Left & directs)        res |= v[7];
    if(Direction::Center & directs)      res |= v[8];

    return res;
}

int AroundGrounds::directionsAroundGround(int ground) const
{
    int res = 0;
    const QVector<int> & v = *this;

    if(v[0] & ground) res |= Direction::TopLeft;
    if(v[1] & ground) res |= Direction::Top;
    if(v[2] & ground) res |= Direction::TopRight;
    if(v[3] & ground) res |= Direction::Right;
    if(v[4] & ground) res |= Direction::BottomRight;
    if(v[5] & ground) res |= Direction::Bottom;
    if(v[6] & ground) res |= Direction::BottomLeft;
    if(v[7] & ground) res |= Direction::Left;

    return res;
}

struct GroundCount : QPair<int, int>
{
    GroundCount() : QPair<int, int>(Ground::Unknown, 0) {}
    GroundCount(int ground, int count) : QPair<int, int>(ground, count) {}
    //bool operator== (int ground) const { return first == ground; }
};

int AroundGrounds::preferablyGround(void) const
{
    QVector<GroundCount> res;
    GroundCount pair;

    res.push_back(GroundCount(Ground::Desert, std::count(begin(), end(), static_cast<int>(Ground::Desert))));
    res.push_back(GroundCount(Ground::Snow, std::count(begin(), end(), static_cast<int>(Ground::Snow))));
    res.push_back(GroundCount(Ground::Swamp, std::count(begin(), end(), static_cast<int>(Ground::Swamp))));
    res.push_back(GroundCount(Ground::Wasteland, std::count(begin(), end(), static_cast<int>(Ground::Wasteland))));
    res.push_back(GroundCount(Ground::Beach, std::count(begin(), end(), static_cast<int>(Ground::Beach))));
    res.push_back(GroundCount(Ground::Lava, std::count(begin(), end(), static_cast<int>(Ground::Lava))));
    res.push_back(GroundCount(Ground::Dirt, std::count(begin(), end(), static_cast<int>(Ground::Dirt))));
    res.push_back(GroundCount(Ground::Grass, std::count(begin(), end(), static_cast<int>(Ground::Grass))));
    res.push_back(GroundCount(Ground::Water, std::count(begin(), end(), static_cast<int>(Ground::Water))));

    for(QVector<GroundCount>::const_iterator
	it = res.begin(); it != res.end(); ++it)
	    if((*it).second > pair.second) pair = *it;

    return pair.first;
}

/* return pair, first: index tile, second: shape - 0: none, 1: vert, 2: horz, 3: both */
QPair<int, int> EditorTheme::groundBoundariesFix(const MapTile & tile, const MapTiles & tiles)
{
    const int ground = tile.groundType();

    AroundGrounds around(tiles, tile.mapPos());

    /*
	1. water - any ground (startGroundTile(ground))
	2. ground - other ground (startGroundTile(ground))
	3. ground - water (+16)
    */

    const int groundAND = around.directionsAroundGround(ground);
    const int groundNOT = around.directionsAroundGround(Ground::All & ~ground);

    switch(ground)
    {
	case Ground::Dirt:
	    if(around.directionsAroundGround(Ground::All & ~(ground | Ground::Water | Ground::Beach)))
		return QPair<int, int>(startFilledTile(ground), 0);
	    else
		return groundBoundariesFixBase(around, ground, 0, groundAND, groundNOT);
	    break;

	case Ground::Beach:
	    return QPair<int, int>(-1, 0);

	case Ground::Water:
	    return groundBoundariesFixBase(around, ground, Ground::Water, groundAND, groundNOT);

	default: break;
    }

    return groundBoundariesFixBase(around, ground, Ground::Water | Ground::Beach, groundAND, groundNOT);
}

int EditorTheme::groundOneTileFix(const MapTile & tile, const MapTiles & tiles)
{
    const int ground = tile.groundType();

    AroundGrounds around(tiles, tile.mapPos());

    const int groundAND = around.directionsAroundGround(ground);

    if(!(IS_EQUAL_VALS(Direction::Top | Direction::Left | Direction::TopLeft, groundAND) ||
	IS_EQUAL_VALS(Direction::Top | Direction::Right | Direction::TopRight, groundAND) ||
	IS_EQUAL_VALS(Direction::Bottom | Direction::Left | Direction::BottomLeft, groundAND) ||
	IS_EQUAL_VALS(Direction::Bottom | Direction::Right | Direction::BottomRight, groundAND)) &&
       !(IS_EQUAL_VALS(Direction::Top | Direction::Left | Direction::Bottom, groundAND) ||
	IS_EQUAL_VALS(Direction::Top | Direction::Right | Direction::Bottom, groundAND) ||
	IS_EQUAL_VALS(Direction::Top | Direction::Left | Direction::Right, groundAND) ||
	IS_EQUAL_VALS(Direction::Left | Direction::Right | Direction::Bottom, groundAND)))
	return around.preferablyGround();

    return Ground::Unknown;
}

QPair<int, int> EditorTheme::groundBoundariesFixBase(const AroundGrounds & around, int ground, int groundMarker, int groundAND, int groundNOT)
{
    // corner: top right
    if(IS_EQUAL_VALS(Direction::All & ~(Direction::TopRight | Direction::Center), groundAND) &&
	IS_EQUAL_VALS(Direction::TopRight, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::TopRight) ?
	    qMakePair(startGroundOriginalTile(ground) + 28, 0) : qMakePair(startGroundOriginalTile(ground) + 12, 0);
    }
    else
    // corner: top left
    if(IS_EQUAL_VALS(Direction::All & ~(Direction::TopLeft | Direction::Center), groundAND) &&
	IS_EQUAL_VALS(Direction::TopLeft, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::TopLeft) ?
	    qMakePair(startGroundOriginalTile(ground) + 28, 2) : qMakePair(startGroundOriginalTile(ground) + 12, 2);
    }
    else
    // corner: bottom right
    if(IS_EQUAL_VALS(Direction::All & ~(Direction::BottomRight | Direction::Center), groundAND) &&
	IS_EQUAL_VALS(Direction::BottomRight, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::BottomRight) ?
	    qMakePair(startGroundOriginalTile(ground) + 28, 1) : qMakePair(startGroundOriginalTile(ground) + 12, 1);
    }
    else
    // corner: bottom left
    if(IS_EQUAL_VALS(Direction::All & ~(Direction::BottomLeft | Direction::Center), groundAND) &&
	IS_EQUAL_VALS(Direction::BottomLeft, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::BottomLeft) ?
	    qMakePair(startGroundOriginalTile(ground) + 28, 3) : qMakePair(startGroundOriginalTile(ground) + 12, 3);
    }
    else
    // top
    if(IS_EQUAL_VALS(Direction::Left | Direction::Right | Direction::Bottom, groundAND) &&
	IS_EQUAL_VALS(Direction::Top, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::Top) ? 
	    qMakePair(startGroundOriginalTile(ground) + 16, 0) : qMakePair(startGroundOriginalTile(ground), 0);
    }
    else
    // bottom
    if(IS_EQUAL_VALS(Direction::Left | Direction::Right | Direction::Top, groundAND) &&
	IS_EQUAL_VALS(Direction::Bottom, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::Bottom) ? 
	    qMakePair(startGroundOriginalTile(ground) + 16, 1) : qMakePair(startGroundOriginalTile(ground), 1);
    }
    else
    // right
    if(IS_EQUAL_VALS(Direction::Left | Direction::Top | Direction::Bottom, groundAND) &&
	IS_EQUAL_VALS(Direction::Right, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::Right) ? 
	    qMakePair(startGroundOriginalTile(ground) + 24, 0) : qMakePair(startGroundOriginalTile(ground) + 8, 0);
    }
    else
    // left
    if(IS_EQUAL_VALS(Direction::Right | Direction::Top | Direction::Bottom, groundAND) &&
	IS_EQUAL_VALS(Direction::Left, groundNOT))
    {
	return groundMarker & around.groundsDirects(Direction::Left) ? 
	    qMakePair(startGroundOriginalTile(ground) + 24, 2) : qMakePair(startGroundOriginalTile(ground) + 8, 2);
    }
    else
    // corner: top + top right + right
    if(IS_EQUAL_VALS(Direction::Left | Direction::Bottom | Direction::BottomLeft, groundAND) &&
	IS_EQUAL_VALS(Direction::Top | Direction::Right, groundNOT))
    {
	if((groundMarker & around.groundsDirects(Direction::Top)) && (groundMarker & around.groundsDirects(Direction::Right)))
	    return qMakePair(startGroundOriginalTile(ground) + 20, 0);
	else
	if(groundMarker & around.groundsDirects(Direction::Top))
	    return qMakePair(startGroundTile(ground) + 36, 0);
	else
	if(groundMarker & around.groundsDirects(Direction::Right))
	    return qMakePair(startGroundTile(ground) + 37, 0);
	else
	    return qMakePair(startGroundOriginalTile(ground) + 4, 0);
    }
    else
    // corner: top + top left + left
    if(IS_EQUAL_VALS(Direction::Right | Direction::Bottom | Direction::BottomRight, groundAND) &&
	IS_EQUAL_VALS(Direction::Top | Direction::Left, groundNOT))
    {
	if((groundMarker & around.groundsDirects(Direction::Top)) && (groundMarker & around.groundsDirects(Direction::Left)))
	    return qMakePair(startGroundOriginalTile(ground) + 20, 2);
	else
	if(groundMarker & around.groundsDirects(Direction::Top))
	    return qMakePair(startGroundTile(ground) + 36, 2);
	else
	if(groundMarker & around.groundsDirects(Direction::Left))
	    return qMakePair(startGroundTile(ground) + 37, 2);
	else
	    return qMakePair(startGroundOriginalTile(ground) + 4, 2);
    }
    else
    // corner: bottom + bottom right + right
    if(IS_EQUAL_VALS(Direction::Left | Direction::Top | Direction::TopLeft, groundAND) &&
	IS_EQUAL_VALS(Direction::Bottom | Direction::Right, groundNOT))
    {
	if((groundMarker & around.groundsDirects(Direction::Bottom)) && (groundMarker & around.groundsDirects(Direction::Right)))
	    return qMakePair(startGroundOriginalTile(ground) + 20, 1);
	else
	if(groundMarker & around.groundsDirects(Direction::Bottom))
	    return qMakePair(startGroundTile(ground) + 36, 1);
	else
	if(groundMarker & around.groundsDirects(Direction::Right))
	    return qMakePair(startGroundTile(ground) + 37, 1);
	else
	    return qMakePair(startGroundOriginalTile(ground) + 4, 1);
    }
    else
    // corner: bottom + bottom left + left
    if(IS_EQUAL_VALS(Direction::Right | Direction::Top | Direction::TopRight, groundAND) &&
	IS_EQUAL_VALS(Direction::Bottom | Direction::Left, groundNOT))
    {
	if((groundMarker & around.groundsDirects(Direction::Bottom)) && (groundMarker & around.groundsDirects(Direction::Left)))
	    return qMakePair(startGroundOriginalTile(ground) + 20, 3);
	else
	if(groundMarker & around.groundsDirects(Direction::Bottom))
	    return qMakePair(startGroundTile(ground) + 36, 3);
	else
	if(groundMarker & around.groundsDirects(Direction::Left))
	    return qMakePair(startGroundTile(ground) + 37, 3);
	else
	    return qMakePair(startGroundOriginalTile(ground) + 4, 3);
    }
    else
    // filled
    if(IS_EQUAL_VALS(Direction::All & ~Direction::Center, groundAND))
    {
	return qMakePair(startFilledTile(ground), 0);
    }

    return qMakePair(-1, 0);
}

QDomElement & operator<< (QDomElement & el, const MapObject & obj)
{
    el.setAttribute("uid", obj.objUid);
    el << static_cast<const QPoint &>(obj);
    return el;
}

QDomElement & operator>> (QDomElement & el, MapObject & obj)
{
    obj.objUid = el.hasAttribute("uid") ? el.attribute("uid").toInt() : 0;
    el >> static_cast<QPoint &>(obj);
    return el;
}

QDomElement & operator<< (QDomElement & el, const Troops & troops)
{
    for(Troops::const_iterator
	it = troops.begin(); it != troops.end(); ++it)
    {
	QDomElement troopElem = el.ownerDocument().createElement("troop");
	el.appendChild(troopElem);
	troopElem.setAttribute("type", (*it).type());
	troopElem.setAttribute("count", (*it).count());
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, Troops & troops)
{
    troops.clear();

    QDomNodeList troopList = el.elementsByTagName("troop");
    for(int pos = 0; pos < troopList.size(); ++pos)
    {
	QDomElement troopElem = troopList.item(pos).toElement();
	int type = troopElem.hasAttribute("type") ? troopElem.attribute("type").toInt() : 0;
	int count = troopElem.hasAttribute("count") ? troopElem.attribute("count").toInt() : 0;
	troops.push_back(Troop(type, count));
    }

    return el;
}

QDomElement & operator<< (QDomElement & el, const Skills & skills)
{
    for(Skills::const_iterator
	it = skills.begin(); it != skills.end(); ++it)
    {
	QDomElement skillElem = el.ownerDocument().createElement("skill");
	el.appendChild(skillElem);
	skillElem.setAttribute("id", (*it).skill());
	skillElem.setAttribute("level", (*it).level());
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, Skills & skills)
{
    skills.clear();

    QDomNodeList skillList = el.elementsByTagName("skill");
    for(int pos = 0; pos < skillList.size(); ++pos)
    {
	QDomElement skillElem = skillList.item(pos).toElement();
	int id = skillElem.hasAttribute("id") ? skillElem.attribute("id").toInt() : 0;
	int level = skillElem.hasAttribute("level") ? skillElem.attribute("level").toInt() : 0;
	if(id && level) skills.push_back(Skill(id, level));
    }

    return el;
}

int Troops::validCount(void) const
{
    return std::count_if(begin(), end(), std::mem_fun_ref(&Troop::isValid));
}

MapTown::MapTown(const QPoint & pos, quint32 id)
    : MapObject(pos, id, MapObj::Castle), col(Color::None), race(Race::Unknown),
	buildings(0), dwellings(0), isCastle(false), forceTown(false), captainPresent(false), customTroops(false), customBuildings(false), customDwellings(false)
{
}

MapTown::MapTown(const QPoint & pos, quint32 id, const mp2town_t & mp2)
    : MapObject(pos, id, MapObj::Castle), buildings(0), dwellings(0), nameTown(mp2.name), isCastle(false), forceTown(mp2.forceTown), captainPresent(false),
	customTroops(mp2.customTroops), customBuildings(mp2.customBuilding), customDwellings(false)
{
    switch(mp2.color)
    {
        case 0:	col = Color::Blue; break;
        case 1: col = Color::Green; break;
        case 2: col = Color::Red; break;
        case 3: col = Color::Yellow; break;
        case 4: col = Color::Orange; break;
        case 5: col = Color::Purple; break;
        default: col = Color::None; break;
    }

    switch(mp2.race)
    {
        case 0: race = Race::Knight; break;
        case 1: race = Race::Barbarian; break;
        case 2: race = Race::Sorceress; break;
        case 3: race = Race::Warlock; break;
        case 4: race = Race::Wizard; break;
        case 5: race = Race::Necromancer; break;
        default: race = Race::Random; break;
    }

    if(mp2.customBuilding)
    {
	if(0x00000002 & mp2.building) buildings |= Building::ThievesGuild;
        if(0x00000004 & mp2.building) buildings |= Building::Tavern;
	if(0x00000008 & mp2.building) buildings |= Building::Shipyard;
        if(0x00000010 & mp2.building) buildings |= Building::Well;
        if(0x00000080 & mp2.building) buildings |= Building::Statue;
        if(0x00000100 & mp2.building) buildings |= Building::LeftTurret;
        if(0x00000200 & mp2.building) buildings |= Building::RightTurret;
        if(0x00000400 & mp2.building) buildings |= Building::Marketplace;
        if(0x00001000 & mp2.building) buildings |= Building::Moat;
        if(0x00000800 & mp2.building) buildings |= Building::ExtraWel2;
        if(0x00002000 & mp2.building) buildings |= Building::ExtraSpec;
        if(0x00080000 & mp2.building) dwellings |= Building::Dwelling1;
        if(0x00100000 & mp2.building) dwellings |= Building::Dwelling2;
        if(0x00200000 & mp2.building) dwellings |= Building::Dwelling3;
        if(0x00400000 & mp2.building) dwellings |= Building::Dwelling4;
        if(0x00800000 & mp2.building) dwellings |= Building::Dwelling5;
        if(0x01000000 & mp2.building) dwellings |= Building::Dwelling6;
        if(0x02000000 & mp2.building) dwellings |= Building::Upgrade2 | Building::Dwelling2;
        if(0x04000000 & mp2.building) dwellings |= Building::Upgrade3 | Building::Dwelling3;
        if(0x08000000 & mp2.building) dwellings |= Building::Upgrade4 | Building::Dwelling4;
        if(0x10000000 & mp2.building) dwellings |= Building::Upgrade5 | Building::Dwelling5;
        if(0x20000000 & mp2.building) dwellings |= Building::Upgrade6 | Building::Dwelling6;
	customDwellings = 0x3FF8000 & mp2.building;
    }

    if(0 < mp2.magicTower) buildings |= Building::MageGuild1;
    if(1 < mp2.magicTower) buildings |= Building::MageGuild2;
    if(2 < mp2.magicTower) buildings |= Building::MageGuild3;
    if(3 < mp2.magicTower) buildings |= Building::MageGuild4;
    if(4 < mp2.magicTower) buildings |= Building::MageGuild5;

    if(mp2.isCastle)
    {
	buildings |= Building::Castle;
	isCastle = true;
    }

    if(mp2.captainPresent)
    {
	buildings |= Building::Captain;
	captainPresent = true;
    }

    if(mp2.customTroops)
    {
	for(int ii = 0; ii < 5; ++ii)
	    troops[ii] = Troop(mp2.troopId[ii], mp2.troopCount[ii]);
    }
}

void MapTown::updateInfo(int spriteIndex, bool random)
{
    if((spriteIndex % 32) < 16)
    {
	buildings |= Building::Castle;
	isCastle = true;
    }
    else
	isCastle = false;

    if(random)
	race = Race::Random;
    else
    switch(spriteIndex / 32)
    {
	case 0:	race = Race::Knight; break;
	case 1:	race = Race::Barbarian; break;
	case 2:	race = Race::Sorceress; break;
	case 3:	race = Race::Warlock; break;
	case 4:	race = Race::Wizard; break;
	case 5:	race = Race::Necromancer; break;
	case 6:	race = Race::Random; break;
	default: break;
    }

    col = Color::None;

    if(nameTown.isEmpty())
    {
	QStringList names = Editor::townNames();
	nameTown = names[Editor::Rand(names.size())];
    }
}

QDomElement & operator<< (QDomElement & el, const MapTown & town)
{
    el << static_cast<const MapObject &>(town);

    el.setAttribute("name", town.nameTown);
    el.setAttribute("color", town.col);
    el.setAttribute("race", town.race);
    el.setAttribute("buildings", town.buildings);
    el.setAttribute("dwellings", town.dwellings);
    el.setAttribute("customTroops", town.customTroops);
    el.setAttribute("customDwellings", town.customDwellings);
    el.setAttribute("customBuildings", town.customBuildings);
    el.setAttribute("forceTown", town.forceTown);
    el.setAttribute("isCastle", town.isCastle);
    el.setAttribute("captainPresent", town.captainPresent);

    QDomDocument doc = el.ownerDocument();

    if(town.troops.size())
    {
	QDomElement troopsElem = doc.createElement("troops");
	el.appendChild(troopsElem);
	troopsElem << town.troops;
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, MapTown & town)
{
    el >> static_cast<MapObject &>(town);

    town.nameTown = el.hasAttribute("name") ? el.attribute("name") : "Unknown";
    town.col = el.hasAttribute("color") ? el.attribute("color").toInt() : Color::None;
    town.race =  el.hasAttribute("race") ? el.attribute("race").toInt() : Race::Unknown;
    town.buildings = el.hasAttribute("buildings") ? el.attribute("buildings").toInt() : 0;
    town.dwellings = el.hasAttribute("dwellings") ? el.attribute("dwellings").toInt() : 0;
    town.customTroops = el.hasAttribute("customTroops") ? el.attribute("customTroops").toInt() : false;
    town.customDwellings = el.hasAttribute("customDwellings") ? el.attribute("customDwellings").toInt() : false;
    town.customBuildings = el.hasAttribute("customBuildings") ? el.attribute("customBuildings").toInt() : false;
    town.forceTown = el.hasAttribute("forceTown") ? el.attribute("forceTown").toInt() : false;
    town.isCastle = el.hasAttribute("isCastle") ? el.attribute("isCastle").toInt() : false;
    town.captainPresent = el.hasAttribute("captainPresent") ? el.attribute("captainPresent").toInt() : false;

    QDomElement troopsElem = el.firstChildElement("troops");
    troopsElem >> town.troops;

    return el;
}

MapHero::MapHero(const QPoint & pos, quint32 id)
    : MapObject(pos, id, MapObj::Heroes), col(Color::None), race(Race::Unknown),
    portrait(Portrait::Unknown), experience(0), patrolMode(false), patrolSquare(0), jailMode(false), magicBook(false)
{
    nameHero = Portrait::transcribe(portrait);
}

MapHero::MapHero(const QPoint & pos, quint32 id, const mp2hero_t & mp2, int spriteIndex, bool jail)
    : MapObject(pos, id, MapObj::Heroes), col(Color::None), race(Race::Unknown), portrait(Portrait::Unknown), jailMode(jail), magicBook(false), nameHero(mp2.name)
{
    updateInfo(spriteIndex);

    if(mp2.customTroops)
    {
	for(int ii = 0; ii < 5; ++ii)
	    troops[ii] = Troop(mp2.troopId[ii], mp2.troopCount[ii]);
    }

    for(int index = 0; index < 3; ++index)
	if(Artifact::None != mp2.artifacts[index])
	    artifacts.push_back(mp2.artifacts[index]);

    experience = mp2.experience;
    patrolMode = mp2.patrol;
    patrolSquare = mp2.patrolSquare;

    if(mp2.customPortrate)
    {
	portrait = mp2.portrateType + 1;
	if(Portrait::Random <= portrait) portrait = Portrait::Unknown;
    }

    if(mp2.customName)
	nameHero = mp2.name;

    if(nameHero.isEmpty())
	nameHero = race == Race::Random ? "Random" : Portrait::transcribe(portrait);

    if(mp2.customSkills)
    {
	for(int ii = 0; ii < 8; ++ii)
	    if(mp2.skillId[ii] && mp2.skillLevel[ii])
		skills.push_back(Skill(mp2.skillId[ii], mp2.skillLevel[ii]));
    }

    if(jail)
	col = Color::None;
}

QString MapHero::name(void) const
{
    if(jailMode)
	return QString(nameHero).append(" (Jail)");

    return nameHero;
}

bool MapHero::haveMagicBook(void) const
{
    return magicBook;
}

void MapHero::updateInfo(int spriteIndex)
{
    switch(spriteIndex / 7)
    {
	case 0:	col = Color::Blue; break;
	case 1:	col = Color::Green; break;
	case 2:	col = Color::Red; break;
	case 3:	col = Color::Yellow; break;
	case 4:	col = Color::Orange; break;
	case 5:	col = Color::Purple; break;
	default: break;
    }

    switch(spriteIndex % 7)
    {
	case 0:	race = Race::Knight; break;
	case 1:	race = Race::Barbarian; break;
	case 2:	race = Race::Sorceress; break;
	case 3:	race = Race::Warlock; break;
	case 4:	race = Race::Wizard; break;
	case 5:	race = Race::Necromancer; break;
	default:race = Race::Random; break;
    }

    // generate portrait, name
    if(race != Race::Random &&
	portrait == Portrait::Unknown)
    {
	portrait = 1 + (Race::index(race) - 1) * 9 + Editor::Rand(9);
	nameHero = Portrait::transcribe(portrait);
    }

    artifacts.clear();
    spells.clear();

    attack = 0;
    defence = 0;
    power = 0;
    knowledge = 0;

    switch(race)
    {
	case Race::Sorceress: magicBook = true;  spells.push_back(Spell::Bless); break;
	case Race::Warlock: magicBook = true;  spells.push_back(Spell::Curse); break;
	case Race::Wizard: magicBook = true;  spells.push_back(Spell::StoneSkin); break;
	case Race::Necromancer: magicBook = true; spells.push_back(Spell::Haste); break;
	default: break;
    }

    switch(race)
    {
	case Race::Knight: attack = 2; defence = 2; power = 1; knowledge = 1; break;
	case Race::Barbarian: attack = 3; defence = 1; power = 1; knowledge = 1; break;
	case Race::Sorceress: attack = 0; defence = 0; power = 2; knowledge = 3; break;
	case Race::Warlock: attack = 0; defence = 0; power = 3; knowledge = 2; break;
	case Race::Wizard: attack = 0; defence = 1; power = 2; knowledge = 2; break;
	case Race::Necromancer: attack = 1; defence = 0; power = 2; knowledge = 2; break;
	default: break;
    }
}

QDomElement & operator<< (QDomElement & el, const MapHero & hero)
{
    el << static_cast<const MapObject &>(hero);

    el.setAttribute("attack", hero.attack);
    el.setAttribute("defence", hero.defence);
    el.setAttribute("power", hero.power);
    el.setAttribute("knowledge", hero.knowledge);
    el.setAttribute("name", hero.nameHero);
    el.setAttribute("color", hero.col);
    el.setAttribute("race", hero.race);
    el.setAttribute("portrait", hero.portrait);
    el.setAttribute("experience", hero.experience);
    el.setAttribute("patrolMode", hero.patrolMode);
    el.setAttribute("patrolSquare", hero.patrolSquare);
    el.setAttribute("jailMode", hero.jailMode);
    el.setAttribute("haveBook", hero.magicBook);

    QDomDocument doc = el.ownerDocument();

    if(hero.artifacts.size())
    {
	QDomElement artifactsElem = doc.createElement("artifacts");
	el.appendChild(artifactsElem);

	for(QVector<int>::const_iterator
	    it = hero.artifacts.begin(); it != hero.artifacts.end(); ++it)
	{
	    QDomElement artElem = doc.createElement("artifact");
	    artifactsElem.appendChild(artElem);
	    artElem.setAttribute("id", *it);
	}
    }

    if(hero.spells.size())
    {
	QDomElement spellsElem = doc.createElement("spells");
	el.appendChild(spellsElem);

	for(QVector<int>::const_iterator
	    it = hero.spells.begin(); it != hero.spells.end(); ++it)
	{
	    QDomElement spellElem = doc.createElement("spell");
	    spellsElem.appendChild(spellElem);
	    spellElem.setAttribute("id", *it);
	}
    }

    if(hero.skills.size())
    {
	QDomElement skillsElem = doc.createElement("skills");
	el.appendChild(skillsElem);
	skillsElem << hero.skills;
    }

    if(hero.troops.size())
    {
	QDomElement troopsElem = doc.createElement("troops");
	el.appendChild(troopsElem);
	troopsElem << hero.troops;
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, MapHero & hero)
{
    el >> static_cast<MapObject &>(hero);

    hero.attack = el.hasAttribute("attack") ? el.attribute("attack").toInt() : 0;
    hero.defence = el.hasAttribute("defence") ? el.attribute("defence").toInt() : 0;
    hero.power = el.hasAttribute("power") ? el.attribute("power").toInt() : 0;
    hero.knowledge = el.hasAttribute("knowledge") ? el.attribute("knowledge").toInt() : 0;
    hero.nameHero = el.hasAttribute("name") ? el.attribute("name") : "Unknown";
    hero.col = el.hasAttribute("color") ? el.attribute("color").toInt() : Color::None;
    hero.race =  el.hasAttribute("race") ? el.attribute("race").toInt() : Race::Unknown;
    hero.portrait = el.hasAttribute("portrait") ? el.attribute("portrait").toInt() : Portrait::Random;
    hero.experience = el.hasAttribute("experience") ? el.attribute("experience").toInt() : 0;
    hero.patrolMode = el.hasAttribute("patrolMode") ? el.attribute("patrolMode").toInt() : false;
    hero.patrolSquare = el.hasAttribute("patrolSquare") ? el.attribute("patrolSquare").toInt() : 0;
    hero.jailMode = el.hasAttribute("jailMode") ? el.attribute("jailMode").toInt() : false;
    hero.magicBook = el.hasAttribute("haveBook") ? el.attribute("haveBook").toInt() : false;

    hero.artifacts.clear();
    QDomNodeList artList = el.firstChildElement("artifacts").elementsByTagName("artifact");
    for(int pos = 0; pos < artList.size(); ++pos)
    {
	QDomElement artElem = artList.item(pos).toElement();
	if(artElem.hasAttribute("id"))
	    hero.artifacts.push_back(artElem.attribute("id").toInt());
    }

    hero.spells.clear();
    QDomNodeList spellList = el.firstChildElement("spells").elementsByTagName("spell");
    for(int pos = 0; pos < spellList.size(); ++pos)
    {
	QDomElement spellElem = spellList.item(pos).toElement();
	if(spellElem.hasAttribute("id"))
	    hero.spells.push_back(spellElem.attribute("id").toInt());
    }

    QDomElement skillsElem = el.firstChildElement("skills");
    skillsElem >> hero.skills;

    QDomElement troopsElem = el.firstChildElement("troops");
    troopsElem >> hero.troops;

    return el;
}

MapSign::MapSign(const QPoint & pos, quint32 id)
    : MapObject(pos, id, MapObj::Sign)
{
}

MapSign::MapSign(const QPoint & pos, quint32 id, const mp2sign_t & mp2)
    : MapObject(pos, id, MapObj::Sign), message(mp2.text)
{
}

QDomElement & operator<< (QDomElement & el, const MapSign & sign)
{
    el << static_cast<const MapObject &>(sign);
    el.appendChild(el.ownerDocument().createTextNode(sign.message));
    return el;
}

QDomElement & operator>> (QDomElement & el, MapSign & sign)
{
    el >> static_cast<MapObject &>(sign);
    sign.message = el.text();
    return el;
}

MapEvent::MapEvent(const QPoint & pos, quint32 id)
    : MapObject(pos, id, MapObj::Event), artifact(Artifact::None), allowComputer(false),
	cancelAfterFirstVisit(true), colors(0)
{
}

MapEvent::MapEvent(const QPoint & pos, quint32 id, const mp2mapevent_t & mp2)
    : MapObject(pos, id, MapObj::Event), artifact(mp2.artifact), allowComputer(mp2.allowComputer),
	cancelAfterFirstVisit(mp2.cancelAfterFirstVisit), colors(0), message(mp2.text)
{
    resources.wood = mp2.resources[0];
    resources.mercury = mp2.resources[1];
    resources.ore = mp2.resources[2];
    resources.sulfur = mp2.resources[3];
    resources.crystal = mp2.resources[4];
    resources.gems = mp2.resources[5];
    resources.gold = mp2.resources[6];

    if(mp2.colors[0]) colors |= Color::Blue;
    if(mp2.colors[1]) colors |= Color::Red;
    if(mp2.colors[2]) colors |= Color::Green;
    if(mp2.colors[3]) colors |= Color::Yellow;
    if(mp2.colors[4]) colors |= Color::Orange;
    if(mp2.colors[5]) colors |= Color::Purple;
}

QDomElement & operator<< (QDomElement & el, const MapEvent & event)
{
    el << static_cast<const MapObject &>(event);

    QDomDocument doc = el.ownerDocument();

    QDomElement resourcesElem = doc.createElement("resources");
    el.appendChild(resourcesElem);
    resourcesElem << event.resources;

    el.setAttribute("artifact", event.artifact);
    el.setAttribute("colors", event.colors);
    el.setAttribute("allowComputer", event.allowComputer);
    el.setAttribute("cancelAfterFirstVisit", event.cancelAfterFirstVisit);

    el.appendChild(doc.createElement("msg")).appendChild(doc.createTextNode(event.message));

    return el;
}

QDomElement & operator>> (QDomElement & el, MapEvent & event)
{
    el >> static_cast<MapObject &>(event);

    QDomElement resourcesElem = el.firstChildElement("resources");
    resourcesElem >> event.resources;

    event.artifact = el.hasAttribute("artifact") ? el.attribute("artifact").toInt() : 0;
    event.colors = el.hasAttribute("colors") ? el.attribute("colors").toInt() : 0;
    event.allowComputer = el.hasAttribute("allowComputer") ? el.attribute("allowComputer").toInt() : false;
    event.cancelAfterFirstVisit = el.hasAttribute("cancelAfterFirstVisit") ? el.attribute("cancelAfterFirstVisit").toInt() : true;

    QDomElement msgElem = el.firstChildElement("msg");
    event.message = msgElem.text();

    return el;
}

MapSphinx::MapSphinx(const QPoint & pos, quint32 id, const mp2sphinx_t & mp2)
    : MapObject(pos, id, MapObj::Sphinx), artifact(mp2.artifact), answers(mp2.answers), message(mp2.text)
{
    resources.wood = mp2.resources[0];
    resources.mercury = mp2.resources[1];
    resources.ore = mp2.resources[2];
    resources.sulfur = mp2.resources[3];
    resources.crystal = mp2.resources[4];
    resources.gems = mp2.resources[5];
    resources.gold = mp2.resources[6];
}

MapSphinx::MapSphinx(const QPoint & pos, quint32 id)
    : MapObject(pos, id, MapObj::Sphinx), artifact(Artifact::None)
{
}

QDomElement & operator<< (QDomElement & el, const MapSphinx & sphinx)
{
    el << static_cast<const MapObject &>(sphinx);

    QDomDocument doc = el.ownerDocument();

    QDomElement resourcesElem = doc.createElement("resources");
    el.appendChild(resourcesElem);
    resourcesElem << sphinx.resources;

    el.setAttribute("artifact", sphinx.artifact);

    QDomElement answersElem = doc.createElement("answers");
    el.appendChild(answersElem);

    for(QStringList::const_iterator
	it = sphinx.answers.begin(); it != sphinx.answers.end(); ++it)
	answersElem.appendChild(doc.createElement("answer")).appendChild(doc.createTextNode(*it));

    el.appendChild(doc.createElement("msg")).appendChild(doc.createTextNode(sphinx.message));

    return el;
}

QDomElement & operator>> (QDomElement & el, MapSphinx & sphinx)
{
    el >> static_cast<MapObject &>(sphinx);

    QDomElement resourcesElem = el.firstChildElement("resources");
    resourcesElem >> sphinx.resources;

    sphinx.artifact = el.hasAttribute("artifact") ? el.attribute("artifact").toInt() : 0;

    sphinx.answers.clear();
    QDomElement answersElem = el.firstChildElement("answers");
    QDomNodeList list = answersElem.elementsByTagName("answer");

    for(int pos = 0; pos < list.size(); ++pos)
	sphinx.answers << list.item(pos).toElement().text();

    QDomElement msgElem = el.firstChildElement("msg");
    sphinx.message = msgElem.text();

    return el;
}

MapActions::MapActions(const QPoint & pos, quint32 id)
    : MapObject(pos, id, MapObj::None)
{
}


QString MapActions::transcribe(int v)
{
    const char* actionName[] = { "DefaultAction", "Access", "Message", "Resources", "Artifact",
	"Troops", "Morale", "Luck", "Experience", "Skill", "Unknown" };

    return v < Unknown ? actionName[v] : actionName[Unknown];
}

bool MapActions::isDefault(void) const
{
    if(list.empty())
	return true;
    else
    if(1 == list.size())
    {
	const ActionDefault* act = dynamic_cast<const ActionDefault*>(list.front().data());
	return act && act->result;
    }

    return false;
}

DayEvent::DayEvent(const mp2dayevent_t & mp2)
    : allowComputer(mp2.allowComputer), dayFirstOccurent(mp2.dayFirstOccurent),
	daySubsequentOccurrences(mp2.subsequentOccurrences), colors(0), message(mp2.text)
{
    resources.wood = mp2.resources[0];
    resources.mercury = mp2.resources[1];
    resources.ore = mp2.resources[2];
    resources.sulfur = mp2.resources[3];
    resources.crystal = mp2.resources[4];
    resources.gems = mp2.resources[5];
    resources.gold = mp2.resources[6];

    if(mp2.colors[0]) colors |= Color::Blue;
    if(mp2.colors[1]) colors |= Color::Red;
    if(mp2.colors[2]) colors |= Color::Green;
    if(mp2.colors[3]) colors |= Color::Yellow;
    if(mp2.colors[4]) colors |= Color::Orange;
    if(mp2.colors[5]) colors |= Color::Purple;
}

QString DayEvent::header(void) const
{
    QString header;
    QTextStream ts(& header);
    ts << "Day " << dayFirstOccurent << " - " << message;
    return header;
}

QDomElement & operator<< (QDomElement & el, const DayEvent & event)
{
    el.setAttribute("colors", event.colors);
    el.setAttribute("allowComputer", event.allowComputer);

    el.setAttribute("dayFirst", event.dayFirstOccurent);
    el.setAttribute("daySubsequent", event.daySubsequentOccurrences);

    QDomDocument doc = el.ownerDocument();

    QDomElement resourcesElem = doc.createElement("resources");
    el.appendChild(resourcesElem);
    resourcesElem << event.resources;

    el.appendChild(doc.createElement("msg")).appendChild(doc.createTextNode(event.message));

    return el;
}

QDomElement & operator>> (QDomElement & el, DayEvent & event)
{
    event.colors = el.hasAttribute("colors") ? el.attribute("colors").toInt() : 0;
    event.allowComputer = el.hasAttribute("allowComputer") ? el.attribute("allowComputer").toInt() : false;

    event.dayFirstOccurent = el.hasAttribute("dayFirst") ? el.attribute("dayFirst").toInt() : 0;
    event.daySubsequentOccurrences = el.hasAttribute("daySubsequent") ? el.attribute("daySubsequent").toInt() : 0;

    QDomElement resourcesElem = el.firstChildElement("resources");
    resourcesElem >> event.resources;

    QDomElement msgElem = el.firstChildElement("msg");
    event.message = msgElem.text();

    return el;
}

MapObjects::MapObjects()
{
}

QMap<quint32, quint32>
    MapObjects::importObjects(const MapObjects & mo, const QRect & srcrt, const QPoint & dstpt, quint32 curUID)
{
    QMap<quint32, quint32> result;

    for(int yy = 0; yy < srcrt.height(); ++yy)
    {
	for(int xx = 0; xx < srcrt.width(); ++xx)
	{
	    SharedMapObject sharedObj = mo.find(QPoint(xx + srcrt.x(), yy + srcrt.y()));
	    if(sharedObj.data())
	    {
		MapObject* ptr = sharedObj.data()->copy();
		result.insert(ptr->uid(), curUID);
		ptr->setPos(QPoint(xx + dstpt.x(), yy + dstpt.y()));
		ptr->setUID(curUID);
		push_back(SharedMapObject(ptr));
		curUID += 1;
	    }
	}
    }

    return result;
}

void MapObjects::remove(const QPoint & pos)
{
    erase(std::remove(begin(), end(), pos), end());
}

void MapObjects::remove(int uid)
{
    erase(std::remove(begin(), end(), uid), end());
}

SharedMapObject MapObjects::find(const QPoint & pos, bool last) const
{
    if(last)
    {
	SharedMapObject res = NULL;

	for(const_iterator it = begin(); it != end(); ++it)
	    if(*it == pos) res = *it;

	return res;
    }

    const_iterator it = std::find(begin(), end(), pos);
    return it != end() ? *it : NULL;
}

QList<SharedMapObject> MapObjects::list(int type) const
{
    QList<SharedMapObject> result;

    for(const_iterator it = begin(); it != end(); ++it)
	if((*it).data()->type() == type) result.push_back(*it);

    return result;
}

QDomElement & operator<< (QDomElement & el, const MapObjects & objects)
{
    QDomDocument doc = el.ownerDocument();

    for(QList<SharedMapObject>::const_iterator
	it = objects.begin(); it != objects.end(); ++it)
    {
	QDomElement elem = doc.createElement((*it).data()->object());
	el.appendChild(elem);

	switch((*it).data()->type())
	{
	    case MapObj::Castle:  { MapTown* obj = dynamic_cast<MapTown*>((*it).data()); if(obj) elem << *obj; } break;
	    case MapObj::Heroes:  { MapHero* obj = dynamic_cast<MapHero*>((*it).data());	if(obj) elem << *obj; } break;
	    case MapObj::Sign:    { MapSign* obj = dynamic_cast<MapSign*>((*it).data());	if(obj) elem << *obj; } break;
	    case MapObj::Event:   { MapEvent* obj = dynamic_cast<MapEvent*>((*it).data()); if(obj) elem << *obj; } break;
	    case MapObj::Sphinx:  { MapSphinx* obj = dynamic_cast<MapSphinx*>((*it).data()); if(obj) elem << *obj; } break;
	    case MapObj::Resource:{ MapResource* obj = dynamic_cast<MapResource*>((*it).data()); if(obj) elem << *obj; } break;
	    case MapObj::Monster: { MapMonster* obj = dynamic_cast<MapMonster*>((*it).data()); if(obj) elem << *obj; } break;
	    case MapObj::Artifact:{ MapArtifact* obj = dynamic_cast<MapArtifact*>((*it).data()); if(obj) elem << *obj; } break;
	    default:{ MapActions* obj = dynamic_cast<MapActions*>((*it).data()); if(obj) elem << *obj; else elem << *(*it).data(); } break;
	}
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, MapObjects & objects)
{
    QDomDocument doc = el.ownerDocument();
    objects.clear();
    QDomNodeList list = el.childNodes();

    for(int pos = 0; pos < list.size(); ++pos)
    {
	QDomElement elem = list.item(pos).toElement();

	if(elem.tagName() == "town")
	{ MapTown* obj = new MapTown(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "hero")
	{ MapHero* obj = new MapHero(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "sign")
	{ MapSign* obj = new MapSign(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "event")
	{ MapEvent* obj = new MapEvent(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "sphinx")
	{ MapSphinx* obj = new MapSphinx(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "artifact")
	{ MapArtifact* obj = new MapArtifact(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "resource")
	{ MapResource* obj = new MapResource(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "monster")
	{ MapMonster* obj = new MapMonster(); elem >> *obj; objects.push_back(obj); }
	else
	if(elem.tagName() == "actions")
	{ MapActions* obj = new MapActions(); elem >> *obj; objects.push_back(obj); }
    }

    return el;
}

DayEvents::DayEvents()
{
}

QDomElement & operator<< (QDomElement & el, const DayEvents & events)
{
    for(DayEvents::const_iterator
        it = events.begin(); it != events.end(); ++it)
    {
	QDomElement elem = el.ownerDocument().createElement("event");
        el.appendChild(elem);
        elem << *it;
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, DayEvents & events)
{
    QDomDocument doc = el.ownerDocument();
    events.clear();

    QDomNodeList list = el.elementsByTagName("event");
    for(int pos = 0; pos < list.size(); ++pos)
    {
	DayEvent event;
	QDomElement elem = list.item(pos).toElement();
	elem >> event;
	events.push_back(event);
    }

    return el;
}

QDomElement & operator<< (QDomElement & el, const Resources & res)
{
    el.setAttribute("wood", res.wood);
    el.setAttribute("mercury", res.mercury);
    el.setAttribute("ore", res.ore);
    el.setAttribute("sulfur", res.sulfur);
    el.setAttribute("crystal", res.crystal);
    el.setAttribute("gems", res.gems);
    el.setAttribute("gold", res.gold);

    return el;
}

QDomElement & operator>> (QDomElement & el, Resources & res)
{
    res.wood = el.hasAttribute("wood") ? el.attribute("wood").toInt() : 0;
    res.mercury = el.hasAttribute("mercury") ? el.attribute("mercury").toInt() : 0;
    res.ore = el.hasAttribute("ore") ? el.attribute("ore").toInt() : 0;
    res.sulfur = el.hasAttribute("sulfur") ? el.attribute("sulfur").toInt() : 0;
    res.crystal = el.hasAttribute("crystal") ? el.attribute("crystal").toInt() : 0;
    res.gems = el.hasAttribute("gems") ? el.attribute("gems").toInt() : 0;
    res.gold = el.hasAttribute("gold") ? el.attribute("gold").toInt() : 0;

    return el;
}

QDomElement & operator<< (QDomElement & el, const TavernRumors & rumors)
{
    QDomDocument doc = el.ownerDocument();

    for(TavernRumors::const_iterator
        it = rumors.begin(); it != rumors.end(); ++it)
        el.appendChild(doc.createElement("msg")).appendChild(doc.createTextNode(*it));

    return el;
}

QDomElement & operator>> (QDomElement & el, TavernRumors & rumors)
{
    rumors.clear();
    QDomNodeList list = el.elementsByTagName("msg");

    for(int pos = 0; pos < list.size(); ++pos)
	rumors << list.item(pos).toElement().text();

    return el;
}

int SpriteLevel::fromString(const QString & level)
{
    if(level == "bottom")
	return Bottom;
    else
    if(level == "action")
	return Action;
    else
    if(level == "shadow")
	return Top;
    else
    if(level == "top")
	return Top;

    qDebug() << "SpriteLevel::fromString:" << "unknown sprite level";
    return Unknown;
}

CompositeSprite::CompositeSprite(const QString & icn, const QDomElement & elem)
    : spriteICN(EditorTheme::mapICN(icn)), spriteIndex(elem.attribute("index").toInt()), spriteLevel(0), spritePassable(Direction::All), spriteAnimation(0)
{
    if(elem.hasAttribute("icn"))
	spriteICN = EditorTheme::mapICN(EditorTheme::checkICN(elem.attribute("icn")));
    spritePos.setX(elem.attribute("px").toInt());
    spritePos.setY(elem.attribute("py").toInt());

    spriteLevel = SpriteLevel::fromString(elem.attribute("level").toLower());

    if(elem.hasAttribute("passable"))
	spritePassable = elem.attribute("passable").toInt(NULL, 0);

    if(elem.hasAttribute("animation"))
	spriteAnimation = elem.attribute("animation").toInt();
}

CompositeObject::CompositeObject(const QDomElement & elem)
    : name(elem.attribute("name")), size(elem.attribute("width").toInt(), elem.attribute("height").toInt()), classId(elem.attribute("cid").toInt(NULL, 0))
{
    icn = EditorTheme::checkICN(elem.attribute("icn"));

    QDomNodeList list = elem.elementsByTagName("sprite");
    for(int pos = 0; pos < list.size(); ++pos)
	push_back(CompositeSprite(icn, list.item(pos).toElement()));

    if(elem.hasAttribute("hide"))
	name.clear();
}

bool CompositeObject::isValid(void) const
{
    return ! name.isEmpty();
}

CompositeObjectCursor::CompositeObjectCursor(const CompositeObject & obj) : CompositeObject(obj), valid(true)
{
    const QSize & tileSize = EditorTheme::tileSize();
    const QSize areaSize(tileSize.width() * size.width(), tileSize.height() * size.height());

    objectArea = EditorTheme::getImage(obj);
    centerOffset = QPoint(areaSize.width() - tileSize.width(),
				areaSize.height() - tileSize.height());

    // generate passable color map
    passableMap = QPixmap(areaSize);
    passableMap.fill(Qt::transparent);

    QPixmap yellowBound = Editor::pixmapBorder(tileSize - QSize(2, 2), Qt::transparent, QColor(255, 255, 0));
    QPainter paint(& passableMap);

    for(CompositeObject::const_iterator
	it = begin(); it != end(); ++it)
    {
	const QPoint offset((*it).spritePos.x() * tileSize.width(),
				(*it).spritePos.y() * tileSize.height());

	QPixmap tileP = Editor::pixmapBorderPassable((*it).spritePassable);

	switch((*it).spriteLevel)
	{
	    case SpriteLevel::Bottom:
		paint.drawPixmap(offset + QPoint(1, 1), tileP);
		break;

	    case SpriteLevel::Action:
		paint.drawPixmap(offset + QPoint(1, 1), tileP);
		centerOffset = offset;
		break;

	    case SpriteLevel::Top:
		paint.drawPixmap(offset + QPoint(1, 1), yellowBound);
		break;

	    default: break;
	}
    }
}

void CompositeObjectCursor::paint(QPainter & painter, const QPoint & pos, bool allow)
{
    Q_UNUSED(allow);
    scenePos = pos;

    painter.drawPixmap(pos, objectArea);
    painter.drawPixmap(pos, passableMap);
}

QRect CompositeObjectCursor::area(void) const
{
    return QRect(scenePos, objectArea.size());
}

QPoint CompositeObjectCursor::center(void) const
{
    return centerOffset;
}

void CompositeObjectCursor::reset(void)
{
    valid = false;
}

bool CompositeObjectCursor::isValid(void) const
{
    return valid;
}

void CompositeObjectCursor::move(const MapTile & tile)
{
    if(classId == MapObj::Castle)
    {
	int ground = tile.groundType();

	// find race
        QVector<CompositeSprite>::iterator it;
	for(it = begin(); it != end(); ++it)
	    if((*it).spriteICN == ICN::OBJNTOWN) break;
	int race = (*it).spriteIndex / 32;

	QString newName = "castle" + QString::number(race) + QString("_") + QString::number(ground);

	// change ground position
	if(name != newName)
	{
	    for(it = begin(); it != end(); ++it)
	    {
		if((*it).spriteICN == ICN::OBJNTWBA)
		{
		    switch(ground)
		    {
			case Ground::Grass:		(*it).spriteIndex = (*it).spriteIndex % 10;break;
			case Ground::Snow:		(*it).spriteIndex = 10 + (*it).spriteIndex % 10; break;
			case Ground::Swamp:		(*it).spriteIndex = 20 + (*it).spriteIndex % 10; break;
			case Ground::Lava:		(*it).spriteIndex = 30 + (*it).spriteIndex % 10; break;
			case Ground::Desert:		(*it).spriteIndex = 40 + (*it).spriteIndex % 10; break;
			case Ground::Dirt:		(*it).spriteIndex = 50 + (*it).spriteIndex % 10; break;
			case Ground::Wasteland:		(*it).spriteIndex = 60 + (*it).spriteIndex % 10; break;
			case Ground::Water:
			case Ground::Beach:		(*it).spriteIndex = 70 + (*it).spriteIndex % 10; break;
			default: break;
		    }
		}
	    }

	    name = newName;
	    objectArea = EditorTheme::getImage(*this);
	}
    }
}

QString GameCondition::variantString(void) const
{
    QString res;
    if(QVariant::Point == variant().type())
    {
        QPoint pt = variant().toPoint();
        QTextStream ts(& res);
        ts << pt.x() << "," << pt.y();
    }
    else
    if(QVariant::Int == variant().type())
        res = QString::number(variant().toInt());
    return res;
}

int Color::count(int v)
{
    int res = 0;
    if(Blue & v) ++res;
    if(Red & v) ++res;
    if(Green & v) ++res;
    if(Yellow & v) ++res;
    if(Orange & v) ++res;
    if(Purple & v) ++res;
    return res;
}

QString Color::transcribe(int v)
{
    switch(v)
    {
	case Blue:	return "Blue";
	case Red:	return "Red";
	case Green:	return "Green";
	case Yellow:	return "Yellow";
	case Orange:	return "Orange";
	case Purple:	return "Purple";

	default: break;
    }

    return "Gray";
}

QColor Color::convert(int v)
{
    switch(v)
    {
	case Blue:	return QColor(0, 0, 0xFF);
	case Red:	return QColor(0xFF, 0, 0);
	case Green:	return QColor(0, 0xFF, 0);
	case Yellow:	return QColor(0xFF, 0xFF, 0);
	case Orange:	return QColor(0xFF, 0x66, 0);
	case Purple:	return QColor(0xFF, 0, 0xFF);
	default: break;
    }

    return QColor(100, 100, 100);
}

int Color::index(int v)
{
    switch(v)
    {
	case Blue:	return 1;
	case Green:	return 2;
	case Red:	return 3;
	case Yellow:	return 4;
	case Orange:	return 5;
	case Purple:	return 6;
	default: break;
    }

    return 0;
}

QVector<int> Color::colors(int v)
{
    QVector<int> res;
    res.reserve(6);
    if(Blue & v) res.push_back(Blue);
    if(Green & v) res.push_back(Green);
    if(Red & v) res.push_back(Red);
    if(Yellow & v) res.push_back(Yellow);
    if(Orange & v) res.push_back(Orange);
    if(Purple & v) res.push_back(Purple);
    return res;
}

QDomElement & operator<< (QDomElement & el, const QSize & sz)
{
    el.setAttribute("width", sz.width());
    el.setAttribute("height", sz.height());
    return el;
}

QDomElement & operator>> (QDomElement & el, QSize & sz)
{
    sz.setWidth(el.hasAttribute("width") ? el.attribute("width").toInt() : 0);
    sz.setHeight(el.hasAttribute("height") ? el.attribute("height").toInt() : 0);
    return el;
}

QDomElement & operator<< (QDomElement & el, const QPoint & pt)
{
    el.setAttribute("posx", pt.x());
    el.setAttribute("posy", pt.y());
    return el;
}

QDomElement & operator>> (QDomElement & el, QPoint & pt)
{
    pt.setX(el.hasAttribute("posx") ? el.attribute("posx").toInt() : 0);
    pt.setY(el.hasAttribute("posy") ? el.attribute("posy").toInt() : 0);
    return el;
}

bool MapObj::IsPickup(int obj)
{
    switch(obj)
    {
        case WaterChest:
        case ShipwreckSurviror:
        case FlotSam:
        case Bottle:
        case TreasureChest:
        case AncientLamp:
        case CampFire:
        case Resource:
        case Artifact:
            return true;

        default: break;
    }

    return false;
}

QString MapObj::transcribe(int index)
{
    const char* names[] = { "None", "AlchemyLab", "Sign", "Buoy", "Skeleton", "DaemonCave", "TreasureChest", "FaerieRing", "CampFire", "Fountain",
	"Gazebo", "AncientLamp", "Graveyard", "ArcherHouse", "GoblinHut", "DwarfCott", "PeasantHut", "Unused17", "Unused18", "Event", "DragonCity",
	"LightHouse", "WaterWheel", "Mines", "Monster", "Obelisk", "Oasis", "Resource", "Coast", "SawMill", "Oracle", "Shrine1", "ShipWreck",
	"Unused33", "DesertTent", "Castle", "StoneLights", "WagonCamp", "WaterChest", "WhirlPool", "WindMill", "Artifact", "Reefs", "Boat",
	"RndUltimateArtifact", "RndArtifact", "RndResource", "RndMonster", "RndTown", "RndCastle", "Mermaid", "RndMonster1", "RndMonster2",
	"RndMonster3", "RndMonster4", "Heroes", "Sirens", "HutMagi", "WatchTower", "TreeHouse", "TreeCity", "Ruins", "Fort", "TradingPost",
	"AbandonedMine", "ThatchedHut", "StandingStones", "Idol", "TreeKnowledge", "DoctorHut", "Temple", "HillFort", "HalflingHole", "MercenaryCamp",
	"Shrine2", "Shrine3", "Pyramid", "CityDead", "Excavation", "Sphinx", "Wagon", "Tarpit", "ArtesianSpring", "TrollBridge", "WateringHole",
	"WitchsHut", "Xanadu", "Cave", "Leanto", "MagellanMaps", "FlotSam", "DerelictShip", "ShipwreckSurviror", "Bottle", "MagicWell", "MagicGarden",
	"ObservationTower", "FreemanFoundry", "EyeMagi", "Trees", "Mounts", "Volcano", "Flowers", "Stones", "WaterLake", "Mandrake", "DeadTree", "Stump",
	"Crater", "Cactus", "Mound", "Dune", "LavaPool", "Shrub", "Arena", "BarrowMounds", "RndArtifact1", "RndArtifact2", "RndArtifact3", "Barrier",
	"TravellerTent", "AlchemyTower", "Stables", "Jail", "FireAltar", "AirAltar", "EarthAltar", "WaterAltar" };

    return QString(names[index & 0x7F]);
}

QString Artifact::transcribe(int index)
{
    const char* names[] = { "None", "Ultimate Book of Knowledge", "Ultimate Sword of Dominion", "Ultimate Cloak of Protection", "Ultimate Wand of Magic",
	"Ultimate Shield", "Ultimate Staff", "Ultimate Crown", "Golden Goose", "Arcane Necklace of Magic", "Caster's Bracelet of Magic", "Mage's Ring of Power",
	"Witch's Broach of Magic", "Medal of Valor", "Medal of Courage", "Medal of Honor", "Medal of Distinction", "Fizbin of Misfortune", "Thunder Mace of Dominion",
	"Armored Gauntlets of Protection", "Defender Helm of Protection", "Giant Flail of Dominion", "Ballista of Quickness", "Stealth Shield of Protection",
	"Dragon Sword of Dominion", "Power Axe of Dominion", "Divine Breastplate of Protection", "Minor Scroll of Knowledge", "Major Scroll of Knowledge",
	"Superior Scroll of Knowledge", "Foremost Scroll of Knowledge", "Endless Sack of Gold", "Endless Bag of Gold", "Endless Purse of Gold", "Nomad Boots of Mobility",
	"Traveler's Boots of Mobility", "Lucky Rabbit's Foot", "Golden Horseshoe", "Gambler's Lucky Coin", "Four-Leaf Clover", "True Compass of Mobility",
	"Sailor's Astrolabe of Mobility", "Evil Eye", "Enchanted Hourglass", "Gold Watch", "Skullcap", "Ice Cloak", "Fire Cloak", "Lightning Helm", "Evercold Icicle",
	"Everhot Lava Rock", "Lightning Rod", "Snake-Ring", "Ankh", "Book of Elements", "Elemental Ring", "Holy Pendant", "Pendant of Free Will", "Pendant of Life",
	"Serenity Pendant", "Seeing-eye Pendant", "Kinetic Pendant", "Pendant of Death", "Wand of Negation", "Golden Bow", "Telescope", "Statesman's Quill",
	"Wizard's Hat", "Power Ring", "Ammo Cart", "Tax Lien", "Hideous Mask", "Endless Pouch of Sulfur", "Endless Vial of Mercury", "Endless Pouch of Gems",
	"Endless Cord of Wood", "Endless Cart of Ore", "Endless Pouch of Crystal", "Spiked Helm", "Spiked Shield", "White Pearl", "Black Pearl",
	"Random", "Ultimate Random", "Random 1", "Random 2", "Random 3",
	"Spell Scroll", "Arm of the Martyr", "Breastplate of Anduran", "Broach of Shielding", "Battle Garb of Anduran",
	"Crystal Ball", "Heart of Fire", "Heart of Ice", "Helmet of Anduran", "Holy Hammer", "Legendary Scepter", "Masthead", "Sphere of Negation", "Staff of Wizardry",
	"Sword Breaker", "Sword of Anduran", "Spade of Necromancy", "Unknown" };

    return isValid(index) ? QString(names[index]) : QString(names[Unknown]);
}

QString Artifact::description(int index)
{
    return transcribe(index);
}

bool Artifact::isValid(int index)
{
    return 0 <= index && Unknown > index;
}

QString Building::extraSpec(int race)
{
    const char* names[] = { "Special", "Fortification", "Coliseum", "Rainbow", "Dungeon", "Library", "Storm" };

    switch(race)
    {
	case Race::Knight:	return names[1];
	case Race::Barbarian:	return names[2];
	case Race::Sorceress:	return names[3];
	case Race::Warlock:	return names[4];
	case Race::Wizard:	return names[5];
	case Race::Necromancer:	return names[6];
	default: break;
    }

    return names[0];
}

QString Building::extraWel2(int race)
{
    const char* names[] = { "1st Level Grow", "Farm", "Garbage He", "Crystal Gar", "Waterfall", "Orchard", "Skull Pile" };

    switch(race)
    {
	case Race::Knight:	return names[1];
	case Race::Barbarian:	return names[2];
	case Race::Sorceress:	return names[3];
	case Race::Warlock:	return names[4];
	case Race::Wizard:	return names[5];
	case Race::Necromancer:	return names[6];
	default: break;
    }

    return names[0];
}

int Building::dwellingMap(int race)
{
    switch(race)
    {
	case Race::Knight:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade2 | Upgrade3 | Upgrade4 | Upgrade5 | Upgrade6;
	case Race::Barbarian:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade2 | Upgrade4 | Upgrade5;
	case Race::Sorceress:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade2 | Upgrade3 | Upgrade4;
	case Race::Warlock:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade4 | Upgrade6 | Upgrade7;
	case Race::Wizard:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade3 | Upgrade5 | Upgrade6;
	case Race::Necromancer:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade2 | Upgrade3 | Upgrade4 | Upgrade5;
	case Race::Random:	return Dwelling1 | Dwelling2 | Dwelling3 | Dwelling4 | Dwelling5 | Dwelling6 |
					Upgrade2 | Upgrade3 | Upgrade4 | Upgrade5 | Upgrade6;
	default: break;
    }

    return Unknown;
}

QString Building::description(int type, int race)
{
    const char* names[] = { "Unknown",
        "The Thieves' Guild provides information on enemy players.\nThieves' Guilds can also provide scouting information on enemy towns.",
        "The Tavern increases morale for troops defending the castle.",
        "The Shipyard allows ships to be built.",
        "The Well increases the growth rate of all dwellings by 2 creatures per week.",
        "The Statue increases your town's income by 250 per day.",
        "The Left Turret provides extra firepower during castle combat.",
        "The Right Turret provides extra firepower during castle combat.",
        "The Marketplace can be used to convert one type of resource into another.\nThe more marketplaces you control, the better the exchange rate.",
        "The Moat slows attacking units.\nAny unit entering the moat must end its turn there and becomes more vulnerable to attack.",
        "The Castle improves town defense and increases income to 1000 gold per day.",
        "The Captain's Quarters provides a captain to assist in the castle's defense when no hero is present.",
        "The Mage Guild allows heroes to learn spells and replenish their spell points.",
	"The Shrine increases the necromancy skill of all your necromancers by 10 percent.",

        "The Farm increases production of Peasants by 8 per week.",
        "The Garbage Heap increases production of Goblins by 8 per week.",
        "The Crystal Garden increases production of Sprites by 8 per week.",
        "The Waterfall increases production of Centaurs by 8 per week.",
        "The Orchard increases production of Halflings by 8 per week.",
        "The Skull Pile increases production of Skeletons by 8 per week.",
        "The building increases production of 1st level monster by 8 per week.",

        "The Fortifications increase the toughness of the walls,\nincreasing the number of turns it takes to knock them down.",
        "The Coliseum provides inspiring spectacles to defending troops,\nraising their morale by two during combat.",
        "The Rainbow increases the luck of the defending units by two.",
        "The Dungeon increases the income of the town by 500 / day.",
        "The Library increases the number of spells in the Guild by one for each level of the guild.",
        "The Storm adds +2 to the power of spells of a defending spell caster.",
        "The special building." };

    switch(type)
    {
	case ThievesGuild:	return names[1];
	case Tavern:		return names[2];
	case Shipyard:		return names[3];
	case Well:		return names[4];
	case Statue:		return names[5];
	case LeftTurret:	return names[6];
	case RightTurret:	return names[7];
	case Marketplace:	return names[8];
	case Moat:		return names[9];
	case Castle:		return names[10];
	case Captain:		return names[11];
	case MageGuild:		return names[12];
	case Shrine:		return names[13];

	case ExtraWel2:
	    switch(race)
	    {
		case Race::Knight:	return names[14];
		case Race::Barbarian:	return names[15];
		case Race::Sorceress:	return names[16];
		case Race::Warlock:	return names[17];
		case Race::Wizard:	return names[18];
		case Race::Necromancer: return names[19];
		default:		return names[20];
	    }
	break;
    
	case ExtraSpec:
	    switch(race)
	    {
		case Race::Knight:	return names[21];
		case Race::Barbarian:	return names[22];
		case Race::Sorceress:	return names[23];
		case Race::Warlock:	return names[24];
		case Race::Wizard:	return names[25];
		case Race::Necromancer: return names[26];
		default:		return names[27];
	    }
	break;

	default: break;
    }

    return names[0];
}

QString Speed::transcribe(int speed)
{
    const char* _names[] = { "Standing", "Crawling", "Very Slow", "Slow", "Average", "Fast", "Very Fast", "Ultra Fast", "Blazing", "Instant" };

    switch(speed)
    {
	case Crawling:	return _names[1];
	case VerySlow:	return _names[2];
	case Slow:	return _names[3];
	case Average:	return _names[4];
	case Fast:	return _names[5];
	case VeryFast:	return _names[6];
	case UltraFast:	return _names[7];
	case Blazing:	return _names[8];
	case Instant:	return _names[9];
    }

    return _names[0];
}

int Race::index(int race)
{
    switch(race)
    {
	case Knight:		return 1;
	case Barbarian:		return 2;
	case Sorceress:		return 3;
	case Warlock:		return 4;
	case Wizard:		return 5;
	case Necromancer:	return 6;
	case Random:		return 7;
	case Multi:		return 8;
	default: break;
    }

    return 0;
}

QString Race::transcribe(int race)
{
    const char* names[] = { "Unknown", "Knight", "Barbarian", "Sorceress", "Warlock", "Wizard", "Necromancer",
            "Random", "Multi" };
    return names[index(race)];
}

QString Monster::transcribe(int index)
{
    const char* names[] = { "None",
        "Peasant", "Archer", "Ranger", "Pikeman", "Veteran Pikeman", "Swordsman", "Master Swordsman", "Cavalry", "Champion", "Paladin", "Crusader",
        "Goblin", "Orc", "Orc Chief", "Wolf", "Ogre", "Ogre Lord", "Troll", "War Troll", "Cyclops",
        "Sprite", "Dwarf", "Battle Dwarf", "Elf", "Grand Elf", "Druid", "Greater Druid", "Unicorn", "Phoenix",
        "Centaur", "Gargoyle", "Griffin", "Minotaur", "Minotaur King", "Hydra", "Green Dragon", "Red Dragon", "Black Dragon",
        "Halfling", "Boar", "Iron Golem", "Steel Golem", "Roc", "Mage", "Archmage", "Giant", "Titan",
        "Skeleton", "Zombie", "Mutant Zombie", "Mummy", "Royal Mummy", "Vampire", "Vampire Lord", "Lich", "Power Lich", "Bone Dragon",
        "Rogue", "Nomad", "Ghost", "Genie", "Medusa", "Earth Element", "Air Element", "Fire Element", "Water Element",
        "Random", "Random Level1", "Random Level2", "Random Level3", "Random Level4", "Unknown" };

    return isValid(index) ? names[index] : names[None];
}

QString Monster::tips(int type)
{
    const MonsterStat & stat = Default::monsterStat(type);
    QString res;
    QTextStream ts(& res);

    ts << transcribe(type) << " stats: " << "\n" <<
	"- " << "attack: " << stat.attack << "\n" <<
	"- " << "defense: " << stat.defense << "\n" <<
	"- " << "damage min: " << stat.damageMin << "\n" <<
	"- " << "damage max: " << stat.damageMax << "\n" <<
	"- " << "hp: " << stat.hp << "\n" <<
	"- " << "speed: " << Speed::transcribe(stat.speed) << "\n";

	if(stat.shots)
	    ts << "shots: " << stat.grown << "\n";

    return res;
}

bool Monster::isValid(int index)
{
    return 0 <= index && Unknown > index;
}

QString SkillType::transcribe(int index)
{
    const char* names[] = { "None", "Pathfinding", "Archery", "Logistics", "Scouting", "Diplomacy", "Navigation", "Leadership",
	"Wisdom", "Mysticism", "Luck", "Ballistics", "Eagleeye", "Necromancy", "Estates", "Unknown" };
    return index < Unknown ? names[index] : names[Unknown];
}

QString SkillLevel::transcribe(int index)
{
    const char* names[] = { "Unknown", "Basic", "Advanced", "Expert" };
    return index <= Expert ? names[index] : names[0];
}

QString Skill::name(void) const
{
    return SkillLevel::transcribe(level()) + " " + SkillType::transcribe(skill());
}

QString Skill::description(void) const
{
    return name();
}

QPixmap Skill::pixmap(void) const
{
    return isValid() ? EditorTheme::getImageICN("MINISS.ICN", skill() - 1).first : QPixmap();
}

QDataStream & operator<< (QDataStream & ds, const GameCondition & cond)
{
    int buf[4];

    buf[0] = cond.first;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;

    if(QVariant::Point == cond.second.type())
    {
        QPoint pt = cond.second.toPoint();
	buf[1] = 1;
	buf[2] = pt.x();
	buf[3] = pt.y();
    }
    else
    if(QVariant::Int == cond.second.type())
    {
	buf[1] = 2;
	buf[2] = cond.second.toInt();
    }

    for(int it = 0; it < 4; ++it)
	ds << buf[it];

    return ds;
}

QDataStream & operator>> (QDataStream & ds, GameCondition & cond)
{
    int buf[4];

    for(int it = 0; it < 4; ++it)
	ds >> buf[it];

    cond.first = buf[0];

    if(buf[1] == 1)
	cond.second = QPoint(buf[2], buf[3]);
    else
    if(buf[1] == 2)
	cond.second = buf[2];
    else
	cond.second = QVariant();

    return ds;
}

QString Resources::describe(void) const
{
    QStringList list;

    if(wood)	list << QString("wood(").append(QString::number(wood)).append(")");
    if(mercury)	list << QString("mercury(").append(QString::number(mercury)).append(")");
    if(ore)	list << QString("ore(").append(QString::number(ore)).append(")");
    if(sulfur)	list << QString("sulfur(").append(QString::number(sulfur)).append(")");
    if(crystal)	list << QString("crystal(").append(QString::number(crystal)).append(")");
    if(gems)	list << QString("gems(").append(QString::number(gems)).append(")");
    if(gold)	list << QString("gold(").append(QString::number(gold)).append(")");

    return list.join(",");
}

QString AccessResult::transcribe(void) const
{
    return NULL;
}

QDomElement & operator<< (QDomElement & el, const MapActions & actions)
{
    el << static_cast<const MapObject &>(actions);

    QDomDocument doc = el.ownerDocument();

    for(MapActionList::const_iterator
	it = actions.list.begin(); it != actions.list.end(); ++it)
    {
	QDomElement typeElem = doc.createElement(MapActions::transcribe((*it)->type).toLower());

	switch((*it)->type)
	{
	    case MapActions::DefaultAction:
	    {
		const ActionDefault* act = dynamic_cast<const ActionDefault*>((*it).data());
		if(act) typeElem << *act;
	    }
	    break;

	    case MapActions::Access:
	    {
		const ActionAccess* act = dynamic_cast<const ActionAccess*>((*it).data());
		if(act) typeElem << *act;
	    }

	    case MapActions::Message:
	    {
		const ActionMessage* act = dynamic_cast<const ActionMessage*>((*it).data());
		if(act) typeElem << *act;
	    }

	    case MapActions::Resources:
	    {
		const ActionResources* act = dynamic_cast<const ActionResources*>((*it).data());
		if(act) typeElem << *act;
	    }

	    case MapActions::Artifact:
	    {
		const ActionArtifact* act = dynamic_cast<const ActionArtifact*>((*it).data());
		if(act) typeElem << *act;
	    }

	    case MapActions::Troops:
	    case MapActions::Morale:
	    case MapActions::Luck:
	    case MapActions::Experience:
	    case MapActions::Skill:
	    default: break;
	}

	el.appendChild(typeElem);
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, MapActions & actions)
{
    el >> static_cast<MapObject &>(actions);

    QDomNodeList list = el.childNodes();
    for(int pos = 0; pos < list.size(); ++pos)
    {
	QDomElement elem = list.item(pos).toElement();
	QString tagName = elem.nodeName();
	ActionSimple* ptr = NULL;

	if(tagName == "defaultaction")
	{
	    ActionDefault* act = new ActionDefault();
	    elem >> *act; ptr = act;
	}
	else
	if(tagName == "access")
	{
	    ActionAccess* act = new ActionAccess();
	    elem >> *act; ptr = act;
	}
	else
	if(tagName == "message")
	{
	    ActionMessage* act = new ActionMessage();
	    elem >> *act; ptr = act;
	}
	else
	if(tagName == "resources")
	{
	    ActionResources* act = new ActionResources();
	    elem >> *act; ptr = act;
	}
	else
	if(tagName == "artifact")
	{
	    ActionArtifact* act = new ActionArtifact();
	    elem >> *act; ptr = act;
	}

	if(ptr) actions.list.push_back(SharedActionSimple(ptr));
    }

    return el;
}

QDomElement & operator<< (QDomElement & el, const ActionMessage & am)
{
    el.appendChild(el.ownerDocument().createTextNode(am.message));
    return el;
}

QDomElement & operator>> (QDomElement & el, ActionMessage & am)
{
    am.message = el.text();
    return el;
}

QDomElement & operator<< (QDomElement & el, const ActionDefault & ad)
{
    el << ad.msg;
    el.setAttribute("enabled", ad.result);
    return el;
}

QDomElement & operator>> (QDomElement & el, ActionDefault & ad)
{
    el >> ad.msg;
    ad.result = el.hasAttribute("enabled") && el.attribute("enabled").toInt() == 1;
    return el;
}

QDomElement & operator<< (QDomElement & el, const ActionAccess & aa)
{
    el << aa.msg;
    el.setAttribute("allowPlayers", aa.access.allowPlayers);
    el.setAttribute("allowComputer", aa.access.allowComputer);
    el.setAttribute("cancelAfterFirstVisit", aa.access.cancelAfterFirstVisit);
    return el;
}

QDomElement & operator>> (QDomElement & el, ActionAccess & aa)
{
    el >> aa.msg;
    aa.access.allowPlayers = el.hasAttribute("allowPlayers") ? el.attribute("allowPlayers").toInt() : 0;
    aa.access.allowComputer = el.hasAttribute("allowComputer") && el.attribute("allowComputer").toInt() == 1;
    aa.access.cancelAfterFirstVisit = el.hasAttribute("cancelAfterFirstVisit") && el.attribute("cancelAfterFirstVisit").toInt() == 1;
    return el;
}

QDomElement & operator<< (QDomElement & el, const ActionResources & ar)
{
    el << ar.msg;
    el << ar.resources;
    return el;
}

QDomElement & operator>> (QDomElement & el, ActionResources & ar)
{
    el >> ar.msg;
    el >> ar.resources;
    return el;
}

QDomElement & operator<< (QDomElement & el, const ActionArtifact & aa)
{
    el << aa.msg;
    el.setAttribute("artifact", aa.artifact);
    if(aa.artifact == Artifact::SpellScroll)
	el.setAttribute("spell", aa.spell);
    return el;
}

QDomElement & operator>> (QDomElement & el, ActionArtifact & aa)
{
    el >> aa.msg;
    aa.artifact = el.hasAttribute("artifact") ? el.attribute("artifact").toInt() : Artifact::Unknown;
    aa.spell = el.hasAttribute("spell") ? el.attribute("spell").toInt() : 0;
    return el;
}

int SharedActionSimple::type(void) const
{
    return data() ? data()->type : MapActions::Unknown;
}

QString Spell::transcribe(int index)
{
    const char* names[] = { "None",
            "Fire Ball", "Fire Blast", "Lightning Bolt", "Chain Lightning", "Teleport", "Cure", "Mass Cure", "Resurrect", "Resurrect True", "Haste", "Mass Haste", "Slow", "Mass Slow",
            "Blind", "Bless", "Mass Bless", "Stone Skin", "Steel Skin", "Curse", "Mass Curse", "Holy Word", "Holy Shout", "Anti Magic", "Dispel", "Mass Dispel", "Arrow", "Berserker",
            "Armageddon", "Elemental Storm", "Meteor Shower", "Paralyze", "Hypnotize", "Cold Ray", "Cold Ring", "Disrupting Ray", "Death Ripple", "Death Wave", "Dragon Slayer",
            "Blood Lust", "Animate Dead", "Mirror Image", "Shield", "Mass Shield", "Summon Earth Element", "Summon Air Element", "Summon Fire Element", "Summon Water Element", "Earth Quake",
            "View Mines", "View Resources", "View Artifacts", "View Towns", "View Heroes", "View All", "Identify Hero", "Summon Boat", "Dimension Door", "Town Gate", "Town Portal",
            "Visions", "Haunt", "Set Earth Guardian", "Set Air Guardian", "Set Fire Guardian", "Set Water Guardian",
            "Random", "Random Level1", "Random Level2", "Random Level3", "Random Level4", "Random Level5", "Unknown" };

    return index < Unknown && index >= None ? names[index] : names[None];
}

int Spell::level(int spell)
{
    switch(spell)
    {
	case Bless: case BloodLust: case Cure: case Curse: case Dispel: case Haste: case Arrow: case Slow: case Shield: case StoneSkin:
	case ViewMines: case ViewResources:
	    return 1;

	case Blind: case ColdRay: case DeathRipple: case DisruptingRay: case DragonSlayer: case LightningBolt: case SteelSkin:
	case Visions: case Haunt: case SummonBoat: case ViewArtifacts:
	    return 2;

	case AnimateDead: case AntiMagic: case ColdRing: case DeathWave: case EarthQuake: case FireBall: case HolyWord: case MassCure: case MassBless: case MassHaste: case MassDispel: case Paralyze: case Teleport:
	case ViewTowns: case ViewHeroes: case IdentifyHero:
	    return 3;

        case FireBlast: case ChainLightning: case Resurrect: case MassSlow: case MassCurse: case HolyShout: case Berserker: case ElementalStorm: case MeteorShower: case MassShield:
	case SetEGuardian: case SetAGuardian: case SetFGuardian: case SetWGuardian: case ViewAll: case TownGate:
	    return 4;

	case ResurrectTrue: case Armageddon: case Hypnotize: case MirrorImage: case SummonEElement: case SummonAElement: case SummonFElement: case SummonWElement:
	case DimensionDoor: case TownPortal:

	default : break;
    }

    return 0;
}

QPixmap Spell::pixmap(int spell)
{
    int index = -1;

    switch(spell)
    {
	case FireBall:		index = 8; break;
	case FireBlast:		index = 9; break;
	case LightningBolt:	index = 4; break;
	case ChainLightning:	index = 5; break;
	case Teleport:		index = 10; break;
	case Cure:		index = 6; break;
	case MassCure:		index = 2; break;
	case Resurrect:		index = 13; break;
	case ResurrectTrue:	index = 12; break;
	case Haste:		index = 14; break;
	case MassHaste:		index = 14; break;
	case Slow:		index = 1; break;
	case MassSlow:		index = 1; break;
        case Blind:		index = 21; break;
	case Bless:		index = 7; break;
	case MassBless:		index = 7; break;
	case StoneSkin:		index = 31; break;
	case SteelSkin:		index = 30; break;
	case Curse:		index = 3; break;
	case MassCurse:		index = 3; break;
	case HolyWord:		index = 22; break;
	case HolyShout:		index = 23; break;
	case AntiMagic:		index = 17; break;
	case Dispel:		index = 18; break;
	case MassDispel:	index = 18; break;
	case Arrow:		index = 38; break;
	case Berserker:		index = 19; break;
        case Armageddon:	index = 16; break;
	case ElementalStorm:	index = 11; break;
	case MeteorShower:	index = 24; break;
	case Paralyze:		index = 20; break;
	case Hypnotize:		index = 37; break;
	case ColdRay:		index = 36; break;
	case ColdRing:		index = 35; break;
	case DisruptingRay:	index = 34; break;
	case DeathRipple:	index = 28; break;
	case DeathWave:		index = 29; break;
	case DragonSlayer:	index = 32; break;
        case BloodLust:		index = 27; break;
	case AnimateDead:	index = 25; break;
	case MirrorImage:	index = 26; break;
	case Shield:		index = 15; break;
	case MassShield:	index = 15; break;
	case SummonEElement:	index = 56; break;
	case SummonAElement:	index = 57; break;
	case SummonFElement:	index = 58; break;
	case SummonWElement:	index = 59; break;
	case EarthQuake:	index = 33; break;
        case ViewMines:		index = 39; break;
	case ViewResources:	index = 40; break;
	case ViewArtifacts:	index = 41; break;
	case ViewTowns:		index = 42; break;
	case ViewHeroes:	index = 43; break;
	case ViewAll:		index = 44; break;
	case IdentifyHero:	index = 45; break;
	case SummonBoat:	index = 46; break;
	case DimensionDoor:	index = 47; break;
	case TownGate:		index = 48; break;
	case TownPortal:	index = 49; break;
        case Visions:		index = 50; break;
	case Haunt:		index = 51; break;
	case SetEGuardian:	index = 52; break;
	case SetAGuardian:	index = 53; break;
	case SetFGuardian:	index = 54; break;
	case SetWGuardian:	index = 55; break;

	default: break;
    }

    return index < 0 ? QPixmap() : EditorTheme::getImageICN("SPELLS.ICN", index).first;
}

bool Spell::isBattle(int spell)
{
    switch(spell)
    {
	case ViewMines: case ViewResources:
	case Visions: case Haunt: case SummonBoat: case ViewArtifacts:
	case ViewTowns: case ViewHeroes: case IdentifyHero:
	case SetEGuardian: case SetAGuardian: case SetFGuardian: case SetWGuardian: case ViewAll: case TownGate:
	case DimensionDoor: case TownPortal:

	case None: case Random: case Random1: case Random2: case Random3: case Random4: case Random5: case Unknown:
	    return false;

	default : break;
    }

    return true;
}

QString Spell::tips(int spell)
{
    return transcribe(spell);
}

void MapArtifact::updateInfo(const mp2til_t & til)
{
    if(type == Artifact::SpellScroll)
	spell = til.quantity1;
}

QDomElement & operator<< (QDomElement & el, const MapArtifact & obj)
{
    el << static_cast<const MapObject &>(obj);
    el.setAttribute("type", obj.type);
    el.setAttribute("condition", obj.condition);
    if(obj.type == Artifact::SpellScroll) el.setAttribute("spell", obj.spell);
    return el;
}

QDomElement & operator>> (QDomElement & el, MapArtifact & obj)
{
    el >> static_cast<MapObject &>(obj);
    obj.type = el.hasAttribute("type") ? el.attribute("type").toInt() : 0;
    obj.condition = el.hasAttribute("condition") ? el.attribute("condition").toInt() : 0;
    obj.spell = el.hasAttribute("spell") ? el.attribute("spell").toInt() : 0;
    return el;
}

QDomElement & operator<< (QDomElement & el, const MapResource & obj)
{
    el << static_cast<const MapObject &>(obj);
    el.setAttribute("type", obj.type);
    el.setAttribute("count", obj.count);
    return el;
}

QDomElement & operator>> (QDomElement & el, MapResource & obj)
{
    el >> static_cast<MapObject &>(obj);
    obj.type = el.hasAttribute("type") ? el.attribute("type").toInt() : 0;
    obj.count = el.hasAttribute("count") ? el.attribute("count").toInt() : 0;
    return el;
}

void MapMonster::updateInfo(const mp2til_t & til)
{
    count = (static_cast<quint32>(til.quantity1) << 8) | til.quantity2;
}

QDomElement & operator<< (QDomElement & el, const MapMonster & obj)
{
    el << static_cast<const MapObject &>(obj);
    el.setAttribute("type", obj.type);
    el.setAttribute("condition", obj.condition);
    el.setAttribute("count", obj.count);
    return el;
}

QDomElement & operator>> (QDomElement & el, MapMonster & obj)
{
    el >> static_cast<MapObject &>(obj);
    obj.type = el.hasAttribute("type") ? el.attribute("type").toInt() : 0;
    obj.condition = el.hasAttribute("condition") ? el.attribute("condition").toInt() : 0;
    obj.count = el.hasAttribute("count") ? el.attribute("count").toInt() : 0;
    return el;
}
