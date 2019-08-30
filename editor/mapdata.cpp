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
#include <cmath>
#include <cfloat>
#include <QtGui>
#include <QtWidgets>
#include <QPainter>
#include <QDomDocument>
#include <QDebug>
#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>

#include "engine.h"
#include "global.h"
#include "program.h"
#include "dialogs.h"
#include "mainwindow.h"
#include "mapwindow.h"
#include "mapdata.h"

namespace GraphicsPixmapItemType
{
    enum { None = 0, Tile, Building, Mount, Tree, Resource, Artifact, Monster, Hero, Other };
};

MapTileExt::MapTileExt(int lvl, const mp2lev_t & ext)
    : spriteICN(H2::MP2ICN(ext.object, false)), spriteExt(ext.object), spriteIndex(ext.index), spriteLevelInt(lvl), spriteUID(ext.uniq)
{
}

MapTileExt::MapTileExt(const CompositeSprite & cs, quint32 uid)
    : spriteICN(cs.spriteICN), spriteExt(0), spriteIndex(cs.spriteIndex), spriteLevelInt(0), spriteUID(uid)
{
    if(cs.spriteAnimation)
	spriteExt |= 0x01;
}

bool MapTileExt::isAnimation(const MapTileExt & mte)
{
    return mte.spriteExt & 0x01;
}

bool MapTileExt::sortLevel1(const MapTileExt & mte1, const MapTileExt & mte2)
{
    return (mte1.spriteLevelInt % 4) > (mte2.spriteLevelInt % 4);
}

bool MapTileExt::sortLevel2(const MapTileExt & mte1, const MapTileExt & mte2)
{
    return (mte1.spriteLevelInt % 4) < (mte2.spriteLevelInt % 4);
}

bool MapTileExt::isActionSprite(const MapTileExt & te)
{
    return SpriteLevel::Action == EditorTheme::getSpriteLevel(te.spriteICN, te.spriteIndex);
}

bool MapTileExt::isTown(const MapTileExt & te)
{
    return ICN::OBJNTOWN == te.spriteICN ? true : isRandomTown(te);
}

bool MapTileExt::isFlag32(const MapTileExt & te)
{
    return ICN::FLAG32 == te.spriteICN;
}

bool MapTileExt::isRandomTown(const MapTileExt & te)
{
    return ICN::OBJNTWRD == te.spriteICN && 32 > te.spriteIndex;
}

bool MapTileExt::isJail(const MapTileExt & te)
{
    return ICN::X_LOC2 == te.spriteICN && 0x09 == te.spriteIndex;
}

bool MapTileExt::isMiniHero(const MapTileExt & te)
{
    return ICN::MINIHERO == te.spriteICN;
}

bool MapTileExt::isSign(const MapTileExt & te)
{
    return (ICN::OBJNMUL2 == te.spriteICN && 114 == te.spriteIndex) ||
	    (ICN::OBJNDSRT == te.spriteICN && 119 == te.spriteIndex) ||
	    (ICN::OBJNLAVA == te.spriteICN && 117 == te.spriteIndex) ||
	    (ICN::OBJNSNOW == te.spriteICN && 143 == te.spriteIndex) ||
	    (ICN::OBJNSWMP == te.spriteICN && 140 == te.spriteIndex);
}

bool MapTileExt::isResource(const MapTileExt & te)
{
    return Resource::Unknown != resource(te);
}

int MapTileExt::resource(const MapTileExt & te)
{
    if(ICN::OBJNRSRC == te.spriteICN)
    {
	switch(te.spriteIndex)
	{
	    case 1:	return Resource::Wood;
	    case 3:	return Resource::Mercury;
	    case 5:	return Resource::Ore;
	    case 7:	return Resource::Sulfur;
	    case 9:	return Resource::Crystal;
	    case 11:	return Resource::Gems;
	    case 13:	return Resource::Gold;
	    case 17:	return Resource::Random;
	    default:	break;
	}
    }

    return Resource::Unknown;
}

bool MapTileExt::isMonster(const MapTileExt & te)
{
    return Monster::None != monster(te);
}

int MapTileExt::monster(const MapTileExt & te)
{
    return ICN::MONS32 == te.spriteICN ? 1 + te.spriteIndex : Monster::None;
}

bool MapTileExt::isArtifact(const MapTileExt & te)
{
    return Artifact::None != artifact(te);
}

int MapTileExt::artifact(const MapTileExt & te)
{
    return ICN::OBJNARTI == te.spriteICN && 0 != (te.spriteIndex % 2) ? 1 + te.spriteIndex / 2 : Artifact::None;
}

bool MapTileExt::isButtle(const MapTileExt & te)
{
    return ICN::OBJNWATR == te.spriteICN && 0 == te.spriteIndex;
}

bool MapTileExt::isSphinx(const MapTileExt & te)
{
    return ICN::OBJNDSRT == te.spriteICN && (85 <= te.spriteIndex || 88 <= te.spriteIndex);
}

bool MapTileExt::isMapEvent(const MapTileExt & te)
{
    return ICN::OBJNMUL2 == te.spriteICN && 163 == te.spriteIndex;
}

int MapTileExt::loyaltyObject(const MapTileExt & te)
{
    switch(te.spriteICN)
    {
        case ICN::X_LOC1:
            if(te.spriteIndex == 3) return MapObj::AlchemyTower | MapObj::IsAction;
            else
            if(te.spriteIndex < 3) return MapObj::AlchemyTower;
            else
            if(70 == te.spriteIndex) return MapObj::Arena | MapObj::IsAction;
            else
            if(3 < te.spriteIndex && te.spriteIndex < 72) return MapObj::Arena;
            else
            if(77 == te.spriteIndex) return MapObj::BarrowMounds | MapObj::IsAction;
            else
            if(71 < te.spriteIndex && te.spriteIndex < 78) return MapObj::BarrowMounds;
            else
            if(94 == te.spriteIndex) return MapObj::EarthAltar | MapObj::IsAction;
            else
            if(77 < te.spriteIndex && te.spriteIndex < 112) return MapObj::EarthAltar;
            else
            if(118 == te.spriteIndex) return MapObj::AirAltar | MapObj::IsAction;
            else
            if(111 < te.spriteIndex && te.spriteIndex < 120) return MapObj::AirAltar;
            else
            if(127 == te.spriteIndex) return MapObj::FireAltar | MapObj::IsAction;
            else
            if(119 < te.spriteIndex && te.spriteIndex < 129) return MapObj::FireAltar;
            else
            if(135 == te.spriteIndex) return MapObj::WaterAltar | MapObj::IsAction;
            else
            if(128 < te.spriteIndex && te.spriteIndex < 137) return MapObj::WaterAltar;
            break;

        case ICN::X_LOC2:
            if(te.spriteIndex == 4) return MapObj::Stables | MapObj::IsAction;
            else
            if(te.spriteIndex < 4) return MapObj::Stables;
            else
            if(te.spriteIndex == 9) return MapObj::Jail | MapObj::IsAction;
            else
            if(4 < te.spriteIndex && te.spriteIndex < 10) return MapObj::Jail;
            else
            if(te.spriteIndex == 37) return MapObj::Mermaid | MapObj::IsAction;
            else
            if(9 < te.spriteIndex && te.spriteIndex < 47) return MapObj::Mermaid;
            else
            if(te.spriteIndex == 101) return MapObj::Sirens | MapObj::IsAction;
            else
            if(46 < te.spriteIndex && te.spriteIndex < 111) return MapObj::Sirens;
            else
            if(110 < te.spriteIndex && te.spriteIndex < 136) return MapObj::Reefs;
            break;

        case ICN::X_LOC3:
            if(te.spriteIndex == 30) return MapObj::HutMagi | MapObj::IsAction;
            else
            if(te.spriteIndex < 32) return MapObj::HutMagi;
            else
            if(te.spriteIndex == 50) return MapObj::EyeMagi | MapObj::IsAction;
            else
            if(31 < te.spriteIndex && te.spriteIndex < 59) return MapObj::EyeMagi;
            break;

        default: break;
    }

    return MapObj::None;
}

void MapTileExt::updateMiniHero(MapTileExt & te, int race, int color)
{
    te.spriteIndex = 0;

    switch(color)
    {
        case Color::Blue:          te.spriteIndex = 0; break;
        case Color::Green:         te.spriteIndex = 7; break;
        case Color::Red:           te.spriteIndex = 14; break;
        case Color::Yellow:        te.spriteIndex = 21; break;
        case Color::Orange:        te.spriteIndex = 28; break;
        case Color::Purple:        te.spriteIndex = 35; break;
        default: break;
    }

    switch(race)
    {
        case Race::Knight:          te.spriteIndex += 0; break;
        case Race::Barbarian:       te.spriteIndex += 1; break;
        case Race::Sorceress:       te.spriteIndex += 2; break;
        case Race::Warlock:         te.spriteIndex += 3; break;
        case Race::Wizard:          te.spriteIndex += 4; break;
        case Race::Necromancer:     te.spriteIndex += 5; break;
        default:                    te.spriteIndex += 6; break;
    }
}

void MapTileExt::updateFlagColor(MapTileExt & te, int color)
{
    switch(color)
    {
	case Color::Blue:	te.spriteIndex = te.spriteIndex % 2 ? 1 : 0; break;
	case Color::Green:	te.spriteIndex = te.spriteIndex % 2 ? 3 : 2; break;
	case Color::Red:	te.spriteIndex = te.spriteIndex % 2 ? 5 : 4; break;
	case Color::Yellow:	te.spriteIndex = te.spriteIndex % 2 ? 7 : 6; break;
	case Color::Orange:	te.spriteIndex = te.spriteIndex % 2 ? 9 : 8; break;
	case Color::Purple:	te.spriteIndex = te.spriteIndex % 2 ? 11 : 10; break;
	default:		te.spriteIndex = te.spriteIndex % 2 ? 13 : 12; break;
    }
}

QDomElement & operator<< (QDomElement & el, const MapTileExt & ext)
{
    el.setAttribute("icn", ext.spriteICN);
    el.setAttribute("ext", ext.spriteExt);
    el.setAttribute("index", ext.spriteIndex);
    el.setAttribute("level", ext.spriteLevelInt);
    el.setAttribute("uid", ext.spriteUID);

    return el;
}

QDomElement & operator>> (QDomElement & el, MapTileExt & ext)
{
    ext.spriteICN = el.attribute("icn").toInt();
    ext.spriteExt = el.attribute("ext").toInt();
    ext.spriteIndex = el.attribute("index").toInt();
    ext.spriteLevelInt = el.attribute("level").toInt();
    ext.spriteUID = el.attribute("uid").toInt();

    return el;
}

void MapTileLevels::paint(QPainter & painter, const QPoint & offset, const QPoint & mpos) const
{
    for(const_iterator it = begin(); it != end(); ++it)
	if((*it).icn() != ICN::MINIHERO)
    {
	QPair<QPixmap, QPoint> p1 = EditorTheme::getImageICN((*it).icn(), (*it).index());
        painter.drawPixmap(offset + p1.second, p1.first);

	if(MapTileExt::isAnimation(*it))
	{
	    int anim = H2::isAnimationICN((*it).icn(), (*it).index(), 0);

	    if(0 < anim)
	    {
		QPair<QPixmap, QPoint> p2 = EditorTheme::getImageICN((*it).icn(), anim);
		    painter.drawPixmap(offset + p2.second, p2.first);
	    }
	    else
		qDebug() << "H2::isAnimationICN:" << "incorrect animation" << mpos << "icn:" << (*it).icn() << "index:" << (*it).index();
	}
    }
}

QString MapTileLevels::infoString(void) const
{
    QString str;
    QTextStream ss(& str);

    for(const_iterator it = begin(); it != end(); ++it)
    {
	ss <<
	    "uniq:     " << (*it).uid() << endl <<
	    "sprite:   " << ICN::transcribe((*it).icn()) << ", " <<  (*it).index() << endl <<
	    "level:    " << (*it).level() << (MapTileExt::isActionSprite(*it) ? ", (action)" : "") << endl <<
	    "passable: " << EditorTheme::getSpritePassable((*it).icn(), (*it).index()) << endl;
	    if(it + 1 != end()) ss << "----------"  << endl;
    }

    return str;
}

const MapTileExt* MapTileLevels::findConst(bool (*pf)(const MapTileExt &)) const
{
    const_iterator it = begin();
    for(; it != end(); ++it) if(pf(*it)) break;
    return it != end() ? &(*it) : NULL;
}

MapTileExt* MapTileLevels::find(bool (*pf)(const MapTileExt &))
{
    return const_cast<MapTileExt*>(findConst(pf));
}

QSet<quint32> MapTileLevels::uids(void) const
{
    QSet<quint32> res;
    for(const_iterator it = begin(); it != end(); ++it)
	res.insert((*it).uid());    
    return res;
}

int MapTileLevels::topObjectID(void) const
{
    int id = MapObj::None;

    for(const_iterator it = end(); it != begin(); --it)
    {
	const MapTileExt & ext = *(it - 1);
	id = EditorTheme::getSpriteID(ext.icn(), ext.index());
	if(MapObj::None != id) break;
    }

    return id;
}

quint32 MapTileLevels::topSpriteUID(void) const
{
    return size() ? back().uid() : 0;
}

bool MapTileLevels::removeSprite(quint32 uid)
{
    return removeAll(MapTileExt(uid));
}

void MapTileLevels::changeUIDs(QMap<quint32, quint32> & mapUIDs)
{
    for(iterator itl = begin(); itl != end(); ++itl)
    {
	const QMap<quint32, quint32>::const_iterator itf = mapUIDs.find((*itl).uid());
	if(itf != mapUIDs.end())
	    (*itl).setUID(*itf);
	else
	{
	    (*itl).setUID(mapUIDs[0xFFFFFFFF]);
	    mapUIDs[0xFFFFFFFF] += 1;
	}
    }
}

QDomElement & operator<< (QDomElement & el, const MapTileLevels & levels)
{
    for(MapTileLevels::const_iterator
	it = levels.begin(); it != levels.end(); ++it)
    {
	QDomElement sprite = el.ownerDocument().createElement("sprite");
	el.appendChild(sprite);
	sprite << *it;
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, MapTileLevels & levels)
{
    levels.clear();
    QDomNodeList list = el.elementsByTagName("sprite");

    for(int pos = 0; pos < list.size(); ++pos)
    {
	levels << MapTileExt();    
	QDomElement sprite = list.item(pos).toElement();
	sprite >> levels.back();
    }

    return el;
}

MapTile::MapTile(const mp2til_t & mp2, const QPoint & pos)
    : mpos(pos), tileSprite(mp2.tileSprite), tileShape(mp2.tileShape % 4), objectID(mp2.objectID),
	passableBase(Direction::All), passableLocal(Direction::Unknown)
{
    setGraphicsPixmapItemValues();
    loadSpriteLevels(mp2.ext);

    // fix MP2 objects
    switch(mp2.objectID)
    {
	// Shrub2
        case 0x38:      objectID = MapObj::Shrub; break;
	// Nothing Special
        case 0x39:      objectID = MapObj::None; break;
        case 0xE9:      objectID = MapObj::Reefs; break;
	// change treasurechest to waterchest
        case 0x86:      objectID = (EditorTheme::ground(tileSprite) != Ground::Water ? MapObj::TreasureChest : MapObj::WaterChest) | MapObj::IsAction; break;
	// fix loyalty obj
        case 0x79:
        case 0x7A:
        case 0xF9:
        case 0xFA:	objectID = MapObj::None; break;

        default: break;
    }

    if(objectID == MapObj::None)
    {
	for(MapTileLevels::const_iterator
	    it = spritesLevel1.begin(); it != spritesLevel1.end() && objectID == MapObj::None; ++it)
	    objectID = MapTileExt::loyaltyObject(*it);

	if(objectID == MapObj::None)
	{
	    for(MapTileLevels::const_iterator
		it = spritesLevel2.begin(); it != spritesLevel2.end() && objectID == MapObj::None; ++it)
		objectID = MapTileExt::loyaltyObject(*it);
	}
    }

    updatePassable();
}

MapTile::MapTile(const MapTile & other)
    : QGraphicsPixmapItem(), mpos(other.mpos), tileSprite(other.tileSprite), tileShape(other.tileShape), objectID(other.objectID),
	spritesLevel1(other.spritesLevel1), spritesLevel2(other.spritesLevel2), passableBase(other.passableBase), passableLocal(other.passableLocal)
{
    setGraphicsPixmapItemValues();
}

MapTile::MapTile(const QPoint & pos)
    : QGraphicsPixmapItem(), mpos(pos), tileSprite(0), tileShape(0), objectID(MapObj::None), passableBase(Direction::All), passableLocal(Direction::Unknown)
{
    setGraphicsPixmapItemValues();
}

MapTile & MapTile::operator=(const MapTile & other)
{
    mpos = other.mpos;
    tileSprite = other.tileSprite;
    tileShape = other.tileShape;
    objectID = other.objectID;

    spritesLevel1 = other.spritesLevel1;
    spritesLevel2 = other.spritesLevel2;

    passableBase = other.passableBase;
    passableLocal = other.passableLocal;

    setGraphicsPixmapItemValues();

    if(objectID == MapObj::None)
	updateObjectID();

    return *this;
}

void MapTile::importTile(const MapTile & other, QMap<quint32, quint32> & mapUIDs)
{
    tileSprite = other.tileSprite;
    tileShape = other.tileShape;
    objectID = other.objectID;

    spritesLevel1 = other.spritesLevel1;
    spritesLevel2 = other.spritesLevel2;

    spritesLevel1.changeUIDs(mapUIDs);
    spritesLevel2.changeUIDs(mapUIDs);

    passableBase = other.passableBase;
    passableLocal = other.passableLocal;

    setGraphicsPixmapItemValues();

    if(objectID == MapObj::None)
	updateObjectID();
}

void MapTile::setGraphicsPixmapItemValues(void)
{
    const QSize & tileSize = EditorTheme::tileSize();
    setOffset(mpos.x() * tileSize.width(), mpos.y() * tileSize.height());
    setFlags(QGraphicsItem::ItemIsSelectable);
    setTileSprite(tileSprite, tileShape);
    setData(0, GraphicsPixmapItemType::Tile);
}

void MapTile::setTileSprite(int index, int rotate)
{
    tileSprite = index;
    tileShape = rotate % 4;

    QPixmap sprite = EditorTheme::getImageTIL("GROUND32.TIL", tileSprite);

    switch(tileShape)
    {
	case 1: setPixmap(sprite.transformed(QTransform().scale( 1, -1))); break;
	case 2: setPixmap(sprite.transformed(QTransform().scale(-1,  1))); break;
	case 3: setPixmap(sprite.transformed(QTransform().scale(-1, -1))); break;
	default: setPixmap(sprite); break;
    }
}

QRectF MapTile::boundingRect(void) const
{
    return QRectF(offset(), pixmap().size());
}

int MapTile::groundType(void) const
{
    return EditorTheme::ground(tileSprite);
}

QString MapTile::indexString(int index)
{
    QString str;

    QTextStream out(& str);
    out.setFieldWidth(3);
    out.setPadChar('0');

    out << index;

    return str;
}

void MapTile::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    // draw tile
    painter->drawPixmap(offset(), pixmap());

    // draw level1
    spritesLevel1.paint(*painter, offset().toPoint(), mpos);

    // draw level2
    spritesLevel2.paint(*painter, offset().toPoint(), mpos);

    MapData* mapData = qobject_cast<MapData*>(scene());

    if(mapData && mapData->showPassableMode())
    {
	int passableCurrent = passableLocal != Direction::Unknown ? passableLocal : passableBase;

	if(Direction::All != passableCurrent &&
	    (!mapData->tileOverMouse || mapData->tileOverMouse != this))
	    painter->drawPixmap(offset() + QPoint(1, 1), Editor::pixmapBorderPassable(passableCurrent));
    }
}

void MapTile::showInfo(void) const
{
    QString msg;
    QTextStream ss2(& msg);

    ss2 << "tile pos:       " << mpos.x() << ", " << mpos.y() << endl \
	<< "tile sprite:    " << tileSprite << endl \
	<< "tile rotate:    " << tileShape << endl \
	<< "base passable:  " << passableBase << endl \
	<< "local passable: " << passableLocal << endl \
	<< "object:         " << MapObj::transcribe(objectID) << endl;


    // draw level2
    if(spritesLevel1.size())
	ss2 << "-------------1--------------" << endl <<
	spritesLevel1.infoString();

    // draw level2
    if(spritesLevel2.size())
	ss2 << "-------------2--------------" << endl <<
	spritesLevel2.infoString();

    ss2 << "----------------------------" << endl;

    QMessageBox::information(NULL, "Tile Info", msg);
}

void MapTile::loadSpriteLevel(MapTileLevels & list, int level, const mp2lev_t & ext)
{
    if(ext.object && ext.index < 0xFF)
    {
	if(ICN::UNKNOWN != H2::MP2ICN(ext.object, true))
	    list << MapTileExt(level, ext);
    }
}

bool MapTile::isObjectEdit(void) const
{
    switch(object())
    {
	case MapObj::Heroes:
    	case MapObj::Jail:
	    return true;

	case MapObj::RndCastle:
	case MapObj::RndTown:
	case MapObj::Castle:
	case MapObj::Bottle:
	case MapObj::Sign:
	case MapObj::Event:
    	case MapObj::Sphinx:
	case MapObj::RndArtifact:
	case MapObj::RndArtifact1:
	case MapObj::RndArtifact2:
	case MapObj::RndArtifact3:
	case MapObj::Artifact:
	case MapObj::RndMonster:
	case MapObj::RndMonster1:
	case MapObj::RndMonster2:
	case MapObj::RndMonster3:
	case MapObj::RndMonster4:
	case MapObj::Monster:
	case MapObj::Resource:
	    return isObjectAction();

	default: break;
    }

    return false;
}

bool MapTile::isObjectAction(void) const
{
    switch(object())
    {
	case MapObj::Heroes:
	    return false;

	default: break;
    }

    return objectID & MapObj::IsAction;
}

int MapTile::object(void) const
{
    return objectID & ~MapObj::IsAction;
}

void MapTile::loadSpriteLevels(const mp2ext_t & mp2)
{
    // level1
    loadSpriteLevel(spritesLevel1, mp2.quantity, mp2.level1);

    // level2
    loadSpriteLevel(spritesLevel2, mp2.quantity, mp2.level2);

    updatePassable();
}

void MapTile::sortSpritesLevels(void)
{
    qStableSort(spritesLevel1.begin(), spritesLevel1.end(), MapTileExt::sortLevel1);
    qStableSort(spritesLevel2.begin(), spritesLevel2.end(), MapTileExt::sortLevel2);
}

void MapTile::updateObjectID(void)
{
    // is action
    if(spritesLevel1.empty())
    {
	objectID = MapObj::None;

	if(spritesLevel2.size())
	    objectID = spritesLevel2.topObjectID();
    }
    else
    {
	const MapTileExt* ext = spritesLevel1.find(MapTileExt::isActionSprite);
	objectID = ext ? EditorTheme::getSpriteID(ext->icn(), ext->index()) : spritesLevel1.topObjectID();
    }
}

void MapTile::updatePassable(void)
{
    passableBase = Direction::All;

    for(MapTileLevels::const_iterator
	it = spritesLevel1.begin(); it != spritesLevel1.end(); ++it)
    {
	if(MapTileExt::isActionSprite(*it))
	{
	    passableBase = EditorTheme::getSpritePassable((*it).icn(), (*it).index());
	    break;
	}
	else
	    passableBase &= EditorTheme::getSpritePassable((*it).icn(), (*it).index());
    }
}

void MapTile::addSpriteSection(const CompositeSprite & cs, quint32 uid)
{
    if(cs.spriteLevel == SpriteLevel::Top)
	spritesLevel2 << MapTileExt(cs, uid);
    else
	spritesLevel1 << MapTileExt(cs, uid);

    updateObjectID();
    updatePassable();
}

void MapTile::removeSpriteSection(quint32 uid)
{
    bool res1 = spritesLevel1.removeSprite(uid);
    bool res2 = spritesLevel2.removeSprite(uid);
    if(res1 || res2) updateObjectID();
    updatePassable();
}

QSet<quint32> MapTile::uids(void) const
{
    QSet<quint32> res;
    res += spritesLevel1.uids();
    res += spritesLevel2.uids();
    return res;
}

QDomElement & operator<< (QDomElement & el, const MapTile & tile)
{
    el << tile.mpos;

    el.setAttribute("base", tile.passableBase);
    el.setAttribute("local", tile.passableLocal);

    el.setAttribute("tileSprite", tile.tileSprite);
    el.setAttribute("tileShape", tile.tileShape);
    el.setAttribute("objectID", tile.objectID);

    QDomElement el1 = el.ownerDocument().createElement("levels1");
    el.appendChild(el1);
    el1 << tile.spritesLevel1;

    QDomElement el2 = el.ownerDocument().createElement("levels2");
    el.appendChild(el2);
    el2 << tile.spritesLevel2;

    return el;
}

QDomElement & operator>> (QDomElement & el, MapTile & tile)
{
    el >> tile.mpos;

    tile.passableBase = el.hasAttribute("base") ? el.attribute("base").toInt() : Direction::All;
    tile.passableLocal = el.hasAttribute("local") ? el.attribute("local").toInt() : Direction::All;

    tile.tileSprite = el.attribute("tileSprite").toInt();
    tile.tileShape = el.attribute("tileShape").toInt();
    tile.objectID = el.attribute("objectID").toInt();

    QDomElement el1 = el.firstChildElement("levels1");
    el1 >> tile.spritesLevel1;

    QDomElement el2 = el.firstChildElement("levels2");
    el2 >> tile.spritesLevel2;

    tile.setGraphicsPixmapItemValues();

    return el;
}

MapTiles::MapTiles(const QSize & sz) : msize(sz)
{
    reserve(msize.width() * msize.height());

    for(int yy = 0; yy < msize.height(); ++yy)
    {
	for(int xx = 0; xx < msize.width(); ++xx)
    	    *this << MapTile(mp2til_t(), QPoint(xx, yy));
    }
}

QString MapTiles::sizeDescription(void) const
{
    if(msize.width() == msize.height())
    {
	switch(msize.width())
	{
	    case 36:	return "small";
	    case 72:	return "medium";
	    case 108:	return "large";
	    case 144:	return "extra large";
	    default: break;
	}
    }

    return "custom";
}

QRect MapTiles::fixedRect(const QRect & srcrt, const QPoint & dstpt) const
{
    QRect res = srcrt;

    if(dstpt.x() + srcrt.width() > msize.width())
	res.setWidth(msize.width() - dstpt.x() + srcrt.width());

    if(dstpt.y() + srcrt.height() > msize.height())
	res.setHeight(msize.height() - dstpt.y() + srcrt.height());

    return res;
}

int randomGround(void)
{
    switch(Editor::Rand(6))
    {
	case 0:	return Ground::Desert;
	case 1: return Ground::Snow;
	case 2: return Ground::Swamp;
	case 3: return Ground::Wasteland;
	case 4: return Ground::Lava;
	case 5: return Ground::Dirt;
	default: break;
    }

    return Ground::Grass;
}

void MapTiles::generateMap(int step)
{
    // Smoothness controls how smooth the resultant terain is.  Higher = more smooth
    float smoothness = 2.0f;

    int width = msize.width();
    int height = msize.height();

    //int step = 64;

    // h determines the fineness of the scale it is working on.  After every step, h
    // is decreased by a factor of "smoothness"
    float h = 1;

    //float[][] map = new float[width][height];
    float map[width][height];

    // Initialize the grid points
    for(int yy = 0; yy < height; yy += 2 * step)
        for(int xx = 0; xx < width; xx += 2 * step)
	    map[xx][yy] = Editor::RandF(2 * h);

    // do the rest of the magic
    while(step > 0)
    {   
	// Diamond step
	for(int xx = step; xx < width; xx += 2 * step)
	{
	    for(int yy = step; yy < height; yy += 2 * step)
	    {
                float sum = 0;
                int count = 0;

                if(xx - step >= 0 && yy - step >= 0)
                {
                    sum += map[xx - step][yy - step];
                    count++;
                }
                if(xx - step >= 0 && yy + step < height)
                {
                    sum += map[xx - step][yy + step];
                    count++;
                }
                if(xx + step < width && yy - step >= 0)
                {
                    sum += map[xx + step][yy - step];
                    count++;
                }
                if(xx + step < width && yy + step < height)
                {
                    sum += map[xx + step][yy + step];
                    count++;
                }

		map[xx][yy] = sum / count + Editor::RandF(-h, h);
	    }
	}

	// Square step
	for(int xx = 0; xx < width; xx += step)
	{
	    for(int yy = step * (1 - (xx / step) % 2); yy < height; yy += 2 * step)
	    {
		float sum = 0;
		int count = 0;

		if(xx - step >= 0)
		{
		    sum += map[xx - step][yy];
		    count++;
    		}

		if(xx + step < width)
		{
		    sum += map[xx + step][yy];
		    count++;
		}

		if(yy - step >= 0)
		{
		    sum += map[xx][yy - step];
		    count++;
		}

		if(yy + step < height)
		{
		    sum += map[xx][yy + step];
		    count++;
    		}

		map[xx][yy] = count > 0 ? sum / count + Editor::RandF(-h, h) : 0;
	    }
	}

	h /= smoothness;
	step /= 2;
    }

    QVector< QPair<QRect, int> > biome(9);
    const int cw = width / 3;
    const int ch = height / 3;

    biome[0] = qMakePair(QRect(0, 0, cw, ch), randomGround());
    biome[1] = qMakePair(QRect(cw, 0, cw, ch), randomGround());
    biome[2] = qMakePair(QRect(2 * cw, 0, cw, ch), randomGround());
    biome[3] = qMakePair(QRect(0, ch, cw, ch), randomGround());
    biome[4] = qMakePair(QRect(cw, ch, cw, ch), randomGround());
    biome[5] = qMakePair(QRect(2 * cw, ch, cw, ch), randomGround());
    biome[6] = qMakePair(QRect(0, 2 * ch, cw, ch), randomGround());
    biome[7] = qMakePair(QRect(cw, 2 * ch, cw, ch), randomGround());
    biome[8] = qMakePair(QRect(2 * cw, 2 * ch, cw, ch), randomGround());

    for(int xx = 0; xx < width; ++xx)
    {
        for(int yy = 0; yy < height; ++yy)
	{
	    MapTile* tile = this->tile(QPoint(xx, yy));

	    if(tile)
	    {
		// set water
		if(map[xx][yy] < 0.5f)
            	    tile->setTileSprite(EditorTheme::startFilledTile(Ground::Water), 0);
		else
		if(map[xx][yy] < 0.8f)
            	    tile->setTileSprite(EditorTheme::startFilledTile(Ground::Beach), 0);
		else
		// set ground from biome
		{
		    for(QVector< QPair<QRect, int> >::const_iterator
			it = biome.begin(); it != biome.end(); ++it)
		    if((*it).first.contains(xx, yy))
		    {
            		tile->setTileSprite(EditorTheme::startFilledTile((*it).second), 0);
			break;
		    }
		}
	    }
	}
    }

    // fixed fill (one tile)
    for(QVector<MapTile>::iterator
	it = begin(); it != end(); ++it) 
    {
	int ground2 = EditorTheme::groundOneTileFix(*it, *this);

        if(ground2 != Ground::Unknown)
	    (*it).setTileSprite(EditorTheme::startFilledTile(ground2), 0);
    }

    // fixed border
    for(QVector<MapTile>::iterator
	it = begin(); it != end(); ++it) 
    {
        QPair<int, int> indexGroundRotate = EditorTheme::groundBoundariesFix(*it, *this);

        if(0 <= indexGroundRotate.first)
            (*it).setTileSprite(indexGroundRotate.first, indexGroundRotate.second);
    }
}

void MapTiles::importTiles(const MapTiles & tiles, const QRect & srcrt, const QPoint & dstpt, QMap<quint32, quint32> & mapUIDs)
{
    for(int yy = 0; yy < srcrt.height(); ++yy)
    {
	for(int xx = 0; xx < srcrt.width(); ++xx)
	{
	    const MapTile* srcTile = tiles.tileConst(QPoint(srcrt.x() + xx, srcrt.y() + yy));
	    MapTile* dstTile = tile(QPoint(dstpt.x() + xx, dstpt.y() + yy));

	    if(srcTile && dstTile)
		dstTile->importTile(*srcTile, mapUIDs);
	}
    }
}

bool MapTiles::importTiles(const QSize & sz, const QVector<mp2til_t> & mp2Tiles, const QVector<mp2ext_t> & mp2Sprites)
{
    msize = sz;
    reserve(msize.width() * msize.height());

    for(int yy = 0; yy < msize.height(); ++yy)
    {
	for(int xx = 0; xx < msize.width(); ++xx)
	{
	    const mp2til_t & mp2til = mp2Tiles[indexPoint(QPoint(xx, yy))];
	    *this << MapTile(mp2til, QPoint(xx, yy));
	    int ext = mp2til.ext.indexExt;

	    while(ext)
	    {
		if(ext >= mp2Sprites.size())
		{
		    qDebug() << "ext block: out of range" << ext;
    		    return false;
		}

		back().loadSpriteLevels(mp2Sprites[ext]);
		ext = mp2Sprites[ext].indexExt;
	    }

	    back().sortSpritesLevels();
	}
    }

    return true;
}

const MapTile* MapTiles::mapToTileConst(const QPoint & pos) const
{
    for(MapTiles::const_iterator
	it = begin(); it != end(); ++it)
	if((*it).boundingRect().contains(pos)) return &(*it);

    return NULL;
}

MapTile* MapTiles::mapToTile(const QPoint & pos)
{
    return const_cast<MapTile*>(mapToTileConst(pos));
}

const MapTile* MapTiles::tileConst(const QPoint & pos) const
{
    const QVector<MapTile> & mapTiles = *this;
    return isValidPoint(pos) ? & mapTiles[indexPoint(pos)] : NULL;
}

MapTile* MapTiles::tile(const QPoint & pos)
{
    return const_cast<MapTile*>(tileConst(pos));
}

const MapTile* MapTiles::tileFromDirectionConst(const MapTile* tile, int direct) const
{
    return tile ? tileFromDirectionConst(tile->mapPos(), direct) : NULL;
}

MapTile* MapTiles::tileFromDirection(const MapTile* tile, int direct)
{
    return const_cast<MapTile*>(tileFromDirectionConst(tile, direct));
}

const MapTile* MapTiles::tileFromDirectionConst(const QPoint & center, int direct) const
{
    QPoint next(center);

    switch(direct)
    {
	case Direction::Top:         if(center.y()) next.setY(center.y() - 1); break;
    	case Direction::Bottom:      if(center.y() < msize.height()) next.setY(center.y() + 1); break;
    	case Direction::Left:        if(center.x()) next.setX(center.x() - 1); break;
    	case Direction::Right:       if(center.x() < msize.width()) next.setX(center.x() + 1); break;

    	case Direction::TopRight:    return tileFromDirectionConst(tileFromDirectionConst(center, Direction::Top), Direction::Right);
    	case Direction::BottomRight: return tileFromDirectionConst(tileFromDirectionConst(center, Direction::Bottom), Direction::Right);
    	case Direction::BottomLeft:  return tileFromDirectionConst(tileFromDirectionConst(center, Direction::Bottom), Direction::Left);
    	case Direction::TopLeft:     return tileFromDirectionConst(tileFromDirectionConst(center, Direction::Top), Direction::Left);
    	default: break;
    }

    return tileConst(next);
}

MapTile* MapTiles::tileFromDirection(const QPoint & center, int direct)
{
    return const_cast<MapTile*>(tileFromDirectionConst(center, direct));
}

void MapTiles::removeSprites(quint32 uid)
{
    for(iterator it = begin(); it != end(); ++it)
	(*it).removeSpriteSection(uid);
}

int MapTiles::indexPoint(const QPoint & pos) const
{
    return pos.x() + pos.y() * msize.width();
}

bool MapTiles::isValidPoint(const QPoint & pos) const
{
    return QRect(QPoint(0, 0), msize).contains(pos);
}

QDomElement & operator<< (QDomElement & el, const MapTiles & tiles)
{
    el << tiles.msize;

    for(MapTiles::const_iterator
	it = tiles.begin(); it != tiles.end(); ++it)
    {
	QDomElement elem = el.ownerDocument().createElement("tile");
	el.appendChild(elem);
	elem << *it;
    }

    return el;
}

QDomElement & operator>> (QDomElement & el, MapTiles & tiles)
{
    el >> tiles.msize;

    tiles.clear();
    QDomNodeList nodeList = el.elementsByTagName("tile");

    if(tiles.msize.width() * tiles.msize.height() != nodeList.size())
	qCritical() << "read tiles: " << "incorrect array";

    for(int pos = 0; pos < nodeList.size(); ++pos)
    {
	QDomElement elem = nodeList.item(pos).toElement();
	tiles << MapTile();
	elem >> tiles.back();
    }

    return el;
}

void MapArea::importMP2Towns(const QVector<H2::TownPos> & towns)
{
    for(QVector<H2::TownPos>::const_iterator
	it = towns.begin(); it != towns.end(); ++it) if(tiles.isValidPoint((*it).pos()))
    {
	const MapTileExt* ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isTown);
	if(ext) objects.push_back(new MapTown((*it).pos(), ext->uid(), (*it).town()));
    }
}

void MapArea::importMP2Heroes(const QVector<H2::HeroPos> & heroes)
{
    for(QVector<H2::HeroPos>::const_iterator
	it = heroes.begin(); it != heroes.end(); ++it) if(tiles.isValidPoint((*it).pos()))
    {
	const MapTileExt* ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isMiniHero);
	bool jail = false;

	if(!ext)
	{
	    ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isJail);
	    jail = true;
	}

	if(ext)
	    objects.push_back(new MapHero((*it).pos(), ext->uid(), (*it).hero(), ext->index(), jail));
    }
}

void MapArea::importMP2Signs(const QVector<H2::SignPos> & signs)
{
    for(QVector<H2::SignPos>::const_iterator
	it = signs.begin(); it != signs.end(); ++it) if(tiles.isValidPoint((*it).pos()))
    {
	const MapTileExt* ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isSign);
	if(!ext) ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isButtle);
	if(ext) objects.push_back(new MapSign((*it).pos(), ext->uid(), (*it).sign()));
    }
}

void MapArea::importMP2MapEvents(const QVector<H2::EventPos> & events)
{
    for(QVector<H2::EventPos>::const_iterator
	it = events.begin(); it != events.end(); ++it) if(tiles.isValidPoint((*it).pos()))
    {
	const MapTileExt* ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isMapEvent);
	if(ext) objects.push_back(new MapEvent((*it).pos(), ext->uid(), (*it).event()));
    }
}

void MapArea::importMP2SphinxRiddles(const QVector<H2::SphinxPos> & sphinxes)
{
    for(QVector<H2::SphinxPos>::const_iterator
	it = sphinxes.begin(); it != sphinxes.end(); ++it) if(tiles.isValidPoint((*it).pos()))
    {
	const MapTileExt* ext = tiles.tileConst((*it).pos())->levels1Const().findConst(MapTileExt::isSphinx);
	if(ext) objects.push_back(new MapSphinx((*it).pos(), ext->uid(), (*it).sphinx()));
    }
}

void MapArea::importArea(const MapArea & area, const QRect & srcrt, const QPoint & dstpt)
{
    QRect srcrt2 = tiles.fixedRect(srcrt, dstpt);

    QMap<quint32, quint32> mapUIDs = objects.importObjects(area.objects, srcrt2, dstpt, uniq);
    uniq += mapUIDs.size();
    mapUIDs[0xFFFFFFFF] = uniq;

    tiles.importTiles(area.tiles, srcrt2, dstpt, mapUIDs);
    uniq = mapUIDs[0xFFFFFFFF];
}

QDomElement & operator<< (QDomElement & el, const MapArea & area)
{
    QDomElement eobjects = el.ownerDocument().createElement("objects");
    eobjects.setAttribute("lastUID", area.uniq);
    el.appendChild(eobjects);
    eobjects << area.objects;

    QDomElement etiles = el.ownerDocument().createElement("tiles");
    el.appendChild(etiles);
    etiles << area.tiles;

    return el;
}

QDomElement & operator>> (QDomElement & el, MapArea & area)
{
    QDomElement eobjects = el.firstChildElement("objects");
    eobjects >> area.objects;
    area.uniq = eobjects.hasAttribute("lastUID") ? eobjects.attribute("lastUID").toInt() : 0;

    QDomElement etiles = el.firstChildElement("tiles");
    etiles >> area.tiles;

    return el;
}

namespace
{
    QSharedPointer<MapArea>	_selectedArea	= QSharedPointer<MapArea>();
    int				_mapVersion	= FH2ENGINE_CURRENT_VERSION;
}

MapHeader::MapHeader(const MapArea & ma) : mapName("New Map"), mapAuthors("unknown"), mapLicense("unknown"),
    mapDifficulty(Difficulty::Normal), mapKingdomColors(0), mapCompColors(0), mapHumanColors(0), mapStartWithHero(false), mapArea(ma)
{
    resetPlayerRace();
}

void MapHeader::resetPlayerRace(void)
{
    for(int it = 0; it < 6; ++it)
	playersRace[it] = Race::Unknown;
}

void MapHeader::updatePlayerRace(int color, int race)
{
    int index = Color::index(color);
    if(index) playersRace[index - 1] |= race;
}

QDomElement & operator<< (QDomElement & eheader, const MapHeader & data)
{
    QDomDocument doc = eheader.ownerDocument();

    eheader.appendChild(doc.createElement("name")).appendChild(doc.createTextNode(data.mapName));
    eheader.appendChild(doc.createElement("description")).appendChild(doc.createTextNode(data.mapDescription));
    eheader.appendChild(doc.createElement("authors")).appendChild(doc.createTextNode(data.mapAuthors));
    eheader.appendChild(doc.createElement("license")).appendChild(doc.createTextNode(data.mapLicense));

    QByteArray bdata;
    QDataStream ds(&bdata, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    ds << static_cast<int>(FH2ENGINE_CURRENT_VERSION) <<
	// uint: localtime
	QDateTime::currentDateTime().toTime_t() <<
	// map size
	data.mapArea.size().width() << data.mapArea.size().height() <<
	// difficulty
	data.mapDifficulty <<
	// colors
	data.mapKingdomColors << data.mapHumanColors << data.mapCompColors;

    // race players
    for(int it = 0; it < 6; ++it)
	ds << data.playersRace[it];

    // start with hero
    ds << data.mapStartWithHero <<
	// conditions
	data.mapConditionWins << data.mapConditionLoss;

    eheader.appendChild(doc.createElement("info")).appendChild(doc.createTextNode(bdata.toBase64()));

    return eheader;
}

QDomElement & operator>> (QDomElement & eheader, MapHeader & data)
{
    data.mapName = eheader.firstChildElement("name").text();
    data.mapDescription = eheader.firstChildElement("description").text();
    data.mapAuthors = eheader.firstChildElement("authors").text();
    data.mapLicense = eheader.firstChildElement("license").text();

    QByteArray bdata = QByteArray::fromBase64(eheader.firstChildElement("info").text().toLatin1());

    if(bdata.isEmpty())
	return eheader;

    QDataStream ds(bdata);
    ds.setByteOrder(QDataStream::LittleEndian);

    uint localtime;
    int mapw, maph;

    ds >> _mapVersion >>
	// uint: localtime
	localtime >>
	// map size
	mapw >> maph >>
	// difficulty
	data.mapDifficulty >>
	// colors
	data.mapKingdomColors >> data.mapHumanColors >> data.mapCompColors;

    // race players
    for (int it = 0; it < 6; ++it)
	ds >> data.playersRace[it];

    // start with hero
    ds >> data.mapStartWithHero >>
	// conditions
	data.mapConditionWins >> data.mapConditionLoss;

    return eheader;
}

MapData::MapData(MapWindow* parent) : QGraphicsScene(parent), tileOverMouse(NULL),
    mapArea(), mapTiles(mapArea.tiles), mapObjects(mapArea.objects), mapHeader(mapArea), showPassable(false)
{
    connect(this, SIGNAL(dataModified()), parent, SLOT(mapWasModified()));

   // init: copy, paste
    editCopyAct = new QAction(QIcon(":/images/menu_copy.png"), tr("Copy"), this);
    editCopyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    editCopyAct->setEnabled(false);
    connect(editCopyAct, SIGNAL(triggered()), this, SLOT(copyToBuffer()));

    editPasteAct = new QAction(QIcon(":/images/menu_paste.png"), tr("Paste"), this);
    editPasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    editPasteAct->setEnabled(isValidBuffer());
    connect(editPasteAct, SIGNAL(triggered()), this, SLOT(pasteFromBuffer()));

    //
    addObjectAct = new QAction(QIcon(":/images/add_objects.png"), tr("Add object..."), this);
    addObjectAct->setStatusTip(tr("Select map object"));
    connect(addObjectAct, SIGNAL(triggered()), this, SLOT(selectObjectImage()));

    editObjectAttrbAct = new QAction(QIcon(":/images/edit_objects.png"), tr("Edit object..."), this);
    editObjectAttrbAct->setStatusTip(tr("Edit map object"));
    connect(editObjectAttrbAct, SIGNAL(triggered()), this, SLOT(editObjectAttributes()));

    editObjectEventsAct = new QAction(QIcon(":/images/edit_events.png"), tr("Edit events..."), this);
    editObjectEventsAct->setStatusTip(tr("Edit object events"));
    connect(editObjectEventsAct, SIGNAL(triggered()), this, SLOT(editObjectEvents()));

    removeObjectAct = new QAction(QIcon(":/images/clear_objects.png"), tr("Remove object..."), this);
    removeObjectAct->setStatusTip(tr("Remove map object"));
    connect(removeObjectAct, SIGNAL(triggered()), this, SLOT(removeCurrentObject()));

    QAction* curAct;

    // init: fill ground
    fillGroundAct = new QActionGroup(this);

    curAct = new QAction(QIcon(":/images/ground_desert.png"), tr("Desert"), this);
    curAct->setStatusTip(tr("Fill desert ground"));
    curAct->setData(Ground::Desert);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_snow.png"), tr("Snow"), this);
    curAct->setStatusTip(tr("Fill snow ground"));
    curAct->setData(Ground::Snow);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_swamp.png"), tr("Swamp"), this);
    curAct->setStatusTip(tr("Fill swamp ground"));
    curAct->setData(Ground::Swamp);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_wasteland.png"), tr("Wasteland"), this);
    curAct->setStatusTip(tr("Fill wasteland ground"));
    curAct->setData(Ground::Wasteland);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_beach.png"), tr("Beach"), this);
    curAct->setStatusTip(tr("Fill beach ground"));
    curAct->setData(Ground::Beach);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_lava.png"), tr("Lava"), this);
    curAct->setStatusTip(tr("Fill lava ground"));
    curAct->setData(Ground::Lava);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_dirt.png"), tr("Dirt"), this);
    curAct->setStatusTip(tr("Fill dirt ground"));
    curAct->setData(Ground::Dirt);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_grass.png"), tr("Grass"), this);
    curAct->setStatusTip(tr("Fill grass ground"));
    curAct->setData(Ground::Grass);
    fillGroundAct->addAction(curAct);

    curAct = new QAction(QIcon(":/images/ground_water.png"), tr("Water"), this);
    curAct->setStatusTip(tr("Fill water"));
    curAct->setData(Ground::Water);
    fillGroundAct->addAction(curAct);

    connect(fillGroundAct, SIGNAL(triggered(QAction*)), this, SLOT(fillGroundAction(QAction*)));

    // init: clear objects
    clearObjectsAct = new QActionGroup(this);

/*
    curAct = new QAction(tr("Buildings"), this);
    curAct->setStatusTip(tr("Remove buildings"));
    curAct->setEnabled(false);
    curAct->setData(1);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(tr("Mounts/Rocs"), this);
    curAct->setStatusTip(tr("Remove mounts/rocs"));
    curAct->setEnabled(false);
    curAct->setData(2);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(tr("Trees/Shrubs"), this);
    curAct->setStatusTip(tr("Remove trees/shrubs"));
    curAct->setEnabled(false);
    curAct->setData(3);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(tr("Pickup resources"), this);
    curAct->setStatusTip(tr("Remove resources"));
    curAct->setEnabled(false);
    curAct->setData(4);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(tr("Artifacts"), this);
    curAct->setStatusTip(tr("Remove artifacts"));
    curAct->setEnabled(false);
    curAct->setData(5);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(tr("Monsters"), this);
    curAct->setStatusTip(tr("Remove monsters"));
    curAct->setEnabled(false);
    curAct->setData(6);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(tr("Heroes"), this);
    curAct->setStatusTip(tr("Remove heroes"));
    curAct->setEnabled(false);
    curAct->setData(7);
    clearObjectsAct->addAction(curAct);

    curAct = new QAction(this);
    curAct->setSeparator(true);
    clearObjectsAct->addAction(curAct);
*/

    curAct = new QAction(tr("All"), this);
    curAct->setStatusTip(tr("Remove all objects"));
    curAct->setData(10);
    clearObjectsAct->addAction(curAct);

    connect(clearObjectsAct, SIGNAL(triggered(QAction*)), this, SLOT(removeObjectsAction(QAction*)));

    // init other
    editPassableAct = new QAction(QIcon(":/images/edit_cell.png"), tr("Edit passable"), this);
    editPassableAct->setStatusTip(tr("Edit cell passable"));
    connect(editPassableAct, SIGNAL(triggered()), this, SLOT(editPassableDialog()));

    cellInfoAct = new QAction(QIcon(":/images/cell_info.png"), tr("Cell info"), this);
    cellInfoAct->setStatusTip(tr("Show cell info"));
    connect(cellInfoAct, SIGNAL(triggered()), this, SLOT(cellInfoDialog()));

    selectAllAct = new QAction(QIcon(":/images/menu_fill.png"), tr("Select All"), this);
    selectAllAct->setStatusTip(tr("Select all tiles"));
    connect(selectAllAct, SIGNAL(triggered()), this, SLOT(selectAllTiles()));
}

const QString & MapData::name(void) const
{
    return mapHeader.mapName;
}

const QString & MapData::description(void) const
{
    return mapHeader.mapDescription;
}

const QString & MapData::authors(void) const
{
    return mapHeader.mapAuthors;
}

const QString & MapData::license(void) const
{
    return mapHeader.mapLicense;
}

int MapData::difficulty(void) const
{
    return mapHeader.mapDifficulty;
}

int MapData::kingdomColors(void) const
{
    return mapHeader.mapKingdomColors;
}

int MapData::humanColors(void) const
{
    return mapHeader.mapHumanColors;
}

int MapData::computerColors(void) const
{
    return mapHeader.mapCompColors;
}

bool MapData::startWithHero(void) const
{
    return mapHeader.mapStartWithHero;
}

const CondWins & MapData::conditionWins(void) const
{
    return mapHeader.mapConditionWins;
}

const CondLoss & MapData::conditionLoss(void) const
{
    return mapHeader.mapConditionLoss;
}

ListStringPos MapData::conditionHeroList(int cond) const
{
    Q_UNUSED(cond);

    ListStringPos res;
    QList<SharedMapObject> listHeroes = mapObjects.list(MapObj::Heroes);

    for(QList<SharedMapObject>::const_iterator
	it = listHeroes.begin(); it != listHeroes.end(); ++it)
        res << QPair<QString, QPoint>((*it).data()->name(), (*it).data()->pos());

    return res;
}

ListStringPos MapData::conditionTownList(int cond) const
{
    Q_UNUSED(cond);

    ListStringPos res;
    QList<SharedMapObject> listCastles = mapObjects.list(MapObj::Castle);

    for(QList<SharedMapObject>::const_iterator
	it = listCastles.begin(); it != listCastles.end(); ++it)
        res << QPair<QString, QPoint>((*it).data()->name(), (*it).data()->pos());

    return res;
}

ListStringPos MapData::conditionArtifactList(void) const
{
    ListStringPos res;

    res << QPair<QString, QPoint>("Ultimate Artifact", QPoint(-1, -1));

    for(int art = Artifact::UltimateBook; art < Artifact::Random; ++art)
	res << QPair<QString, QPoint>(Artifact::transcribe(art), QPoint(art, -1));

    for(int art = Artifact::SpellScroll; art < Artifact::Unknown; ++art)
	res << QPair<QString, QPoint>(Artifact::transcribe(art), QPoint(art, -1));

    return res;
}

QList<QString> MapData::conditionSideList(void) const
{
    return QList<QString>() << "Left vs Right" << "Right vs Left";
}

const QSize & MapData::size(void) const
{
    return mapTiles.mapSize();
}

const QStringList & MapData::tavernRumorsList(void) const
{
    return tavernRumors;
}

void MapData::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // clear selected
    if(selectedItems().size())
    {
	if((event->buttons() & Qt::LeftButton) ||
	    ((event->buttons() & Qt::RightButton) && ! selectionArea().contains(event->scenePos())))
	{
	    clearSelection();
	    _selectedArea.clear();
	}
    }
    else
    // place current object
    if(currentObject.isValid() &&
	(event->buttons() & Qt::LeftButton))
    {
        addMapObject(currentObject.scenePos, currentObject, mapArea.uid());
	update(currentObject.area());
    }

    event->accept();
}

void MapData::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // reset current object
    if(currentObject.isValid())
    {
	currentObject.reset();
	update(currentObject.area());
    }
    else
    // context menu
    {
	QMenu menu;

	if(selectedItems().size())
	{
	    editCopyAct->setEnabled(selectedItems().size());

    	    menu.addAction(editCopyAct);
    	    menu.addSeparator();

    	    QMenu* groundSubMenu = menu.addMenu(QIcon(":/images/menu_fill.png"), tr("Fill Ground"));
    	    QList<QAction*> actions = fillGroundAct->actions();

    	    for(QList<QAction*>::const_iterator
        	it = actions.begin(); it != actions.end(); ++it)
        	groundSubMenu->addAction(*it);

    	    menu.addSeparator();

    	    QMenu* clearSubMenu = menu.addMenu(QIcon(":/images/clear_objects.png"), tr("Remove Objects"));
    	    actions = clearObjectsAct->actions();

    	    for(QList<QAction*>::const_iterator
        	it = actions.begin(); it != actions.end(); ++it)
        	clearSubMenu->addAction(*it);
	}
	else
	{
	    editPasteAct->setEnabled(isValidBuffer());

	    menu.addAction(editPasteAct);
    	    menu.addSeparator();

    	    menu.addAction(addObjectAct);

    	    menu.addSeparator();
    	    menu.addAction(editPassableAct);
    	    menu.addAction(editObjectAttrbAct);
    	    menu.addAction(editObjectEventsAct);
    	    menu.addAction(removeObjectAct);
    	    menu.addAction(cellInfoAct);

    	    editObjectAttrbAct->setEnabled(tileOverMouse && tileOverMouse->isObjectEdit());
    	    editObjectEventsAct->setEnabled(tileOverMouse && tileOverMouse->isObjectAction());
    	    removeObjectAct->setEnabled(tileOverMouse && tileOverMouse->isObjectAction());

    	    menu.addSeparator();
    	    menu.addAction(selectAllAct);
	}

	if(selectAllAct != menu.exec(event->screenPos()))
    	    clearSelection();
    }

    event->accept();
}

void MapData::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    MapTile* newTileOverMouse = itemAtAsTile(event->scenePos());

    if(currentObject.isValid())
    {
	if(newTileOverMouse) currentObject.move(*newTileOverMouse);
	update(currentObject.area());
    }
    else
    // select area
    if(event->buttons() & Qt::LeftButton)
    {
	if(selectedItems().size())
	    clearSelection();

	selectArea(event->buttonDownScenePos(Qt::LeftButton), event->scenePos());
    }

    // update select item over cursor
    if(tileOverMouse)
	update(tileOverMouse->boundingRect());


    if(newTileOverMouse)
    {
	if(tileOverMouse != newTileOverMouse)
	{
	    if(!tileOverMouse ||
		tileOverMouse->mapPos().x() != newTileOverMouse->mapPos().x() ||
		tileOverMouse->mapPos().y() != newTileOverMouse->mapPos().y())
		    emit currentTilePosChanged(newTileOverMouse);

	    tileOverMouse = newTileOverMouse;
	}

	if(tileOverMouse)
	    update(tileOverMouse->boundingRect());
    }

    event->accept();
}

void MapData::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    if(tileOverMouse)
    {
	if(tileOverMouse->isObjectEdit())
	    editObjectAttributes();
	else
	if(tileOverMouse->isObjectAction())
	    editObjectEvents();
	else
	    cellInfoDialog();
    }

    event->accept();
}

void MapData::selectArea(QPointF ptdn, QPointF ptup)
{
    if(ptup != ptdn)
    {
	if(ptup.x() < ptdn.x())
	    qSwap(ptup.rx(), ptdn.rx());

	if(ptup.y() < ptdn.y())
	    qSwap(ptup.ry(), ptdn.ry());

	QRect selRect = QRectF(ptdn, ptup).toRect();

	const QSize & tileSize = EditorTheme::tileSize();

	int sl = selRect.left() / tileSize.width();
	int st = selRect.top() / tileSize.height();
	int sr = selRect.right() / tileSize.width();
	int sb = selRect.bottom() / tileSize.height();

	if(selRect.left() > sl * tileSize.width())
	    selRect.setLeft(sl * tileSize.width());

	if(selRect.top() > st * tileSize.height())
	    selRect.setTop(st * tileSize.height());

	if(selRect.right() > sr * tileSize.width())
	    selRect.setRight((sr + 1) * tileSize.width() - 1);

	if(selRect.bottom() > sb * tileSize.height())
	    selRect.setBottom((sb + 1) * tileSize.height() - 1);

	QPainterPath path;
	path.addRect(selRect);

	setSelectionArea(path);
    }
}

void MapData::selectAllTiles(void)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const QSize & sz = EditorTheme::tileSize();
    selectArea(QPointF(0, 0), QPointF(sz.width() * size().width(), sz.height() * size().height()));
    QApplication::restoreOverrideCursor();
}

void MapData::drawForeground(QPainter* painter, const QRectF & rect)
{
    Q_UNUSED(rect);

    // paint: selected area
    if(selectedItems().size())
    {
	painter->setPen(QPen(QColor(40, 40, 100), 1));
	painter->setBrush(QBrush(QColor(40, 40, 100, 150), Qt::Dense4Pattern));
	painter->drawRoundedRect(selectionArea().boundingRect(), 6.0, 6.0);
    }
    else
    if(tileOverMouse)
    {
	// paint: selected new object place
	if(currentObject.isValid())
	{
	    QPoint pos = tileOverMouse->boundingRect().topLeft().toPoint() - currentObject.center();
	    currentObject.paint(*painter, pos, true);
	}
	else
	// paint: selected item over mouse
	{
	    painter->setPen(QPen(tileOverMouse->isObjectAction() ? QColor(0, 255, 0) : QColor(255, 255, 0), 1));
	    painter->setBrush(QBrush(QColor(0, 0, 0, 0)));
	    const QRectF & rt = tileOverMouse->boundingRect();
	    painter->drawRect(QRectF(rt.x(), rt.y(), rt.width() - 1, rt.height() - 1));
	}
    }
}

void MapData::editPassableDialog(void)
{
    if(tileOverMouse)
    {
	Form::EditPassableDialog form(*tileOverMouse);

        if(QDialog::Accepted == form.exec())
	    tileOverMouse->setLocalPassable(form.result());
    }
}

void MapData::cellInfoDialog(void)
{
    if(tileOverMouse)
	tileOverMouse->showInfo();
}

void MapData::copyToBuffer(void)
{
    if(selectedItems().size())
    {
	const QRect srcrt = mapToRect(selectionArea().boundingRect().toRect());
	MapArea* ptr = new MapArea(srcrt.size());
	ptr->importArea(mapArea, srcrt, QPoint(0, 0));
	_selectedArea = QSharedPointer<MapArea>(ptr);
    }
}

bool MapData::isValidBuffer(void) const
{
    return ! _selectedArea.isNull();
}

void MapData::pasteFromBuffer(void)
{
    if(tileOverMouse && isValidBuffer())
    {
	const MapArea & selMapArea = *_selectedArea.data();
	mapArea.importArea(selMapArea, QRect(QPoint(0, 0), selMapArea.size()), tileOverMouse->mapPos());

	updateKingdomColors(0);
	updatePlayersRaces();

	// update heroes sprites
	QList<SharedMapObject> listHeroes = mapArea.objects.list(MapObj::Heroes);

	for(QList<SharedMapObject>::const_iterator
    	    it = listHeroes.begin(); it != listHeroes.end(); ++it)
	{
	    MapHero* hero = dynamic_cast<MapHero*>((*it).data());

	    if(hero)
	    {
		const MapTile* tile = mapTiles.tileConst(hero->pos());
		const MapTileExt* ext = tile ? tile->levels1Const().findConst(MapTileExt::isMiniHero) : NULL;
		if(ext) addHeroItem(tile->mapPos(), *ext);
	    }
	}

	emit dataModified();
    }
}

void MapData::fillGroundAction(QAction* act)
{
    if(act)
    {
	int ground = act->data().toInt();
	QList<QGraphicsItem*> selected = selectedItems();

	// fill default
	for(QList<QGraphicsItem*>::iterator
	    it = selected.begin(); it != selected.end(); ++it)
	{
	    MapTile* tile = qgraphicsitem_cast<MapTile*>(*it);

    	    if(tile)
		tile->setTileSprite(EditorTheme::startFilledTile(ground), 0);
	}

	// fixed fill (one tile)
	for(QList<QGraphicsItem*>::iterator
	    it = selected.begin(); it != selected.end(); ++it)
	{
	    MapTile* tile = qgraphicsitem_cast<MapTile*>(*it);

	    int ground2 = EditorTheme::groundOneTileFix(*tile, mapTiles);
	    if(ground2 != Ground::Unknown)
		tile->setTileSprite(EditorTheme::startFilledTile(ground2), 0);
	}

	// fixed border
	const QSize & tileSize = EditorTheme::tileSize();
        QRectF rectArea = selectionArea().boundingRect();
	QPoint tile2(tileSize.width() * 2, tileSize.height() * 2);

	rectArea.setTopLeft(rectArea.topLeft() - tile2);
	rectArea.setBottomRight(rectArea.bottomRight() + tile2);

	QList<QGraphicsItem*> listItems = items(rectArea, Qt::ContainsItemShape, Qt::AscendingOrder);

	for(QList<QGraphicsItem*>::iterator
	    it = listItems.begin(); it != listItems.end(); ++it)
	{
	    MapTile* tile = qgraphicsitem_cast<MapTile*>(*it);

    	    if(tile)
	    {
		QPair<int, int> indexGroundRotate = EditorTheme::groundBoundariesFix(*tile, mapTiles);

		if(0 <= indexGroundRotate.first)
            	    tile->setTileSprite(indexGroundRotate.first, indexGroundRotate.second);
	    }
	}

	emit dataModified();
    }
}

void MapData::removeObjectsAction(QAction* act)
{
    if(act)
    {
        int type = act->data().toInt();
	QList<QGraphicsItem*> selected = selectedItems();
	QSet<quint32> uids;

	for(QList<QGraphicsItem*>::iterator
	    it = selected.begin(); it != selected.end(); ++it)
	{
	    MapTile* tile = qgraphicsitem_cast<MapTile*>(*it);
	    if(tile) uids += tile->uids();
	}

	bool removeHeroesItems = false;

	switch(type)
	{
/*
	    // remove buildings
	    case 1:
	    // remove mounts/rocs
	    case 2:
	    // remove trees/shrubs
	    case 3:
	    // remove resources
	    case 4:
	    // remove artifacts
	    case 5:
	    // remove monsters
	    case 6:
		break;
	    // remove heroes
	    case 7:
		removeHeroesItems = true;
		break;
*/
	    // Remove all objects
	    case 10:
		removeHeroesItems = true;
		break;

	    default: uids.clear(); break;
	}

	for(QSet<quint32>::const_iterator
	    it = uids.begin(); it != uids.end(); ++it)
	{
	    mapTiles.removeSprites(*it);
	    mapObjects.remove(*it);
	}

	if(removeHeroesItems)
	for(QList<QGraphicsItem*>::iterator
	    it = selected.begin(); it != selected.end(); ++it)
	{
	    MapTile* tile = qgraphicsitem_cast<MapTile*>(*it);
	    if(tile) removeHeroItem(*tile);
	}

	update();
	emit dataModified();
    }

}

void MapData::removeCurrentObject(void)
{
    if(tileOverMouse)
    {
	// remove object info
	MapObject* obj = mapObjects.find(tileOverMouse->mapPos()).data();
	if(obj) mapObjects.remove(obj->uid());

	// remove sprites
	if(! tileOverMouse->levels1Const().empty())
	{
    	    // get uid from level 1 (last sprite)
	    mapTiles.removeSprites(tileOverMouse->levels1Const().back().uid());
	    update();
	}

	removeHeroItem(*tileOverMouse);

	emit dataModified();
    }
}

void MapData::newMap(const QSize & msz, int generate, const QString &, int seq)
{
    const QSize & tileSize = EditorTheme::tileSize();

    mapArea = MapArea(msz);
    mapHeader.mapName = QString("New Map").append(" ").append(QString::number(seq));

    if(0 < generate)
	mapArea.tiles.generateMap(generate);

    // insert tiles
    for(MapTiles::iterator
	it = mapTiles.begin(); it != mapTiles.end(); ++it)
	addItem(& (*it));

    setSceneRect(QRect(QPoint(0, 0),
	QSize(size().width() * tileSize.width(), size().height() * tileSize.height())));

    emit dataModified();
}

bool MapData::loadMap(const QString & mapFile)
{
    qDebug() << "MapData::loadMap:" << mapFile;
    bool load = false;

    if(mapFile.right(4) == ".map")
	load = loadMapXML(mapFile);
    else
    if(mapFile.right(4) == ".mp2" || mapFile.right(4) == ".mx2" ||
	mapFile.right(4) == ".MP2" || mapFile.right(4) == ".MX2")
	load = loadMapMP2(mapFile);

    if(! load && ! loadMapMP2(mapFile) && ! loadMapXML(mapFile))
	return false;

    // insert tiles
    for(MapTiles::iterator
	it = mapTiles.begin(); it != mapTiles.end(); ++it)
	addItem(& (*it));

    // create heroes group
    for(MapTiles::iterator
	it = mapTiles.begin(); it != mapTiles.end(); ++it)
    {
	const MapTileExt* ext = (*it).levels1Const().findConst(MapTileExt::isMiniHero);
	if(ext) addHeroItem((*it).mapPos(), *ext);
    }

    const QSize & tileSize = EditorTheme::tileSize();
    setSceneRect(QRect(QPoint(0, 0),
	QSize(size().width() * tileSize.width(), size().height() * tileSize.height())));

    updatePlayersRaces();

    return true;
}

bool MapData::loadMapMP2(const QString & mapFile)
{
    MP2Format mp2;

    if(! mp2.loadMap(mapFile))
	return false;

    // import tiles
    if(! mapTiles.importTiles(mp2.size, mp2.tiles, mp2.sprites))
	return false;

    mapHeader.mapName = mp2.name;
    mapHeader.mapDescription = mp2.description;
    mapHeader.mapStartWithHero = mp2.startWithHero;

    switch(mp2.difficulty)
    {
	case 0:		mapHeader.mapDifficulty = Difficulty::Easy; break;
	case 2:		mapHeader.mapDifficulty = Difficulty::Tough; break;
	case 3:		mapHeader.mapDifficulty = Difficulty::Expert; break;
	default:	mapHeader.mapDifficulty = Difficulty::Normal; break;
    }

    if(mp2.kingdomColor[0]) mapHeader.mapKingdomColors |= Color::Blue;
    if(mp2.kingdomColor[1]) mapHeader.mapKingdomColors |= Color::Green;
    if(mp2.kingdomColor[2]) mapHeader.mapKingdomColors |= Color::Red;
    if(mp2.kingdomColor[3]) mapHeader.mapKingdomColors |= Color::Yellow;
    if(mp2.kingdomColor[4]) mapHeader.mapKingdomColors |= Color::Orange;
    if(mp2.kingdomColor[5]) mapHeader.mapKingdomColors |= Color::Purple;

    if(mp2.humanAllow[0]) mapHeader.mapHumanColors |= Color::Blue;
    if(mp2.humanAllow[1]) mapHeader.mapHumanColors |= Color::Green;
    if(mp2.humanAllow[2]) mapHeader.mapHumanColors |= Color::Red;
    if(mp2.humanAllow[3]) mapHeader.mapHumanColors |= Color::Yellow;
    if(mp2.humanAllow[4]) mapHeader.mapHumanColors |= Color::Orange;
    if(mp2.humanAllow[5]) mapHeader.mapHumanColors |= Color::Purple;

    if(mp2.compAllow[0]) mapHeader.mapCompColors |= Color::Blue;
    if(mp2.compAllow[1]) mapHeader.mapCompColors |= Color::Green;
    if(mp2.compAllow[2]) mapHeader.mapCompColors |= Color::Red;
    if(mp2.compAllow[3]) mapHeader.mapCompColors |= Color::Yellow;
    if(mp2.compAllow[4]) mapHeader.mapCompColors |= Color::Orange;
    if(mp2.compAllow[5]) mapHeader.mapCompColors |= Color::Purple;

    switch(mp2.conditionWins)
    {
	case 1:		mapHeader.mapConditionWins.set(Conditions::CaptureTown, QPoint(mp2.conditionWinsData3, mp2.conditionWinsData4)); break;
	case 2:		mapHeader.mapConditionWins.set(Conditions::DefeatHero, QPoint(mp2.conditionWinsData3, mp2.conditionWinsData4)); break;
	case 3:		mapHeader.mapConditionWins.set(Conditions::FindArtifact, static_cast<int>(mp2.conditionWinsData3)); break;
	case 4:		mapHeader.mapConditionWins.set(Conditions::SideWins, static_cast<int>(mp2.conditionWinsData3)); break;
	case 5:		mapHeader.mapConditionWins.set(Conditions::AccumulateGold, 1000 * static_cast<int>(mp2.conditionWinsData3)); break;
	default:	mapHeader.mapConditionWins.set(Conditions::Wins); break;
    }

    if(mp2.conditionWinsData1)
	mapHeader.mapConditionWins.first |= Conditions::CompAlsoWins;

    if(mp2.conditionWinsData2)
	mapHeader.mapConditionWins.first |= Conditions::AllowNormalVictory;

    switch(mp2.conditionLoss)
    {
	case 1:		mapHeader.mapConditionLoss.set(Conditions::LoseTown, QPoint(mp2.conditionLossData1, mp2.conditionLossData2)); break;
	case 2:		mapHeader.mapConditionLoss.set(Conditions::LoseHero, QPoint(mp2.conditionLossData1, mp2.conditionLossData2)); break;
	case 3:		mapHeader.mapConditionLoss.set(Conditions::OutTime, static_cast<int>(mp2.conditionLossData1)); break;
	default:	mapHeader.mapConditionLoss.set(Conditions::Loss); break;
    }

    mapArea.uniq = mp2.uniq + 1;

    // import towns
    mapArea.importMP2Towns(mp2.castles);

    // import heroes
    mapArea.importMP2Heroes(mp2.heroes);

    // import signs
    mapArea.importMP2Signs(mp2.signs);

    // import map events
    mapArea.importMP2MapEvents(mp2.mapEvents);

    // import sphinx riddles
    mapArea.importMP2SphinxRiddles(mp2.sphinxes);

    // import day events
    for(QVector<mp2dayevent_t>::const_iterator
	    it = mp2.dayEvents.begin(); it != mp2.dayEvents.end(); ++it)
	    mapDayEvents.push_back(DayEvent(*it));

    // import rumors
    for(QVector<mp2rumor_t>::const_iterator
	    it = mp2.rumors.begin(); it != mp2.rumors.end(); ++it)
	    tavernRumors << Rumor(*it);

    for(MapTiles::const_iterator
	it = mapTiles.begin(); it != mapTiles.end(); ++it)
    if((*it).isObjectAction())
    {
	// import artifacts
	if(MapObj::Artifact == (*it).object() ||
	    MapObj::RndArtifact == (*it).object() ||
	    MapObj::RndArtifact1 == (*it).object() || MapObj::RndArtifact2 == (*it).object() || MapObj::RndArtifact3 == (*it).object())
	{
	    const MapTileExt* ext = mapTiles.tileConst((*it).mapPos())->levels1Const().findConst(MapTileExt::isArtifact);
	    if(ext)
	    {
		MapArtifact* art = new MapArtifact((*it).mapPos(), ext->uid(), MapTileExt::artifact(*ext));
		art->updateInfo(mp2.tiles[it - mapTiles.begin()]);
		mapObjects.push_back(art);
	    }
	}
	else
	// import resources
	if(MapObj::Resource == (*it).object())
	{
	    const MapTileExt* ext = mapTiles.tileConst((*it).mapPos())->levels1Const().findConst(MapTileExt::isResource);
	    if(ext) mapObjects.push_back(new MapResource((*it).mapPos(), ext->uid(), MapTileExt::resource(*ext)));
	}
	else
	// import monsters
	if(MapObj::Monster == (*it).object() ||
	    MapObj::RndMonster == (*it).object() ||
	    MapObj::RndMonster1 == (*it).object() || MapObj::RndMonster2 == (*it).object() || MapObj::RndMonster3 == (*it).object() || MapObj::RndMonster4 == (*it).object())
	{
	    const MapTileExt* ext = mapTiles.tileConst((*it).mapPos())->levels1Const().findConst(MapTileExt::isMonster);
	    if(ext)
	    {
		MapMonster* mons = new MapMonster((*it).mapPos(), ext->uid(), MapTileExt::monster(*ext));
		mons->updateInfo(mp2.tiles[it - mapTiles.begin()]);
		mapObjects.push_back(mons);
	    }
	}
    }

    return true;
}

bool MapData::loadMapXML(const QString & mapFile)
{
    QFile file(mapFile);
    QDomDocument dom;

    if(file.open(QIODevice::ReadOnly))
    {
	QString errorStr;
        int errorLine;
        int errorColumn;

        if(! dom.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
        {
    	    qDebug() << errorStr << errorLine << errorColumn;
            file.close();
            return false;
        }
    }
    else
    { qDebug() << "error open " << mapFile;  return false; }
    file.close();

    QDomElement emap = dom.firstChildElement("map");

    if(emap.isNull())
    { qDebug() << "unknown format map";  return false; }

    QDomElement edata = emap.firstChildElement("data");
    quint16 compress = edata.attribute("compress").toInt();

    if(compress)
    {
	QByteArray bdata;
	QByteArray cdata = QByteArray::fromBase64(edata.text().toLatin1());

        if(compress != qChecksum(cdata.data(), cdata.size()))
	{
	    QApplication::restoreOverrideCursor();
	    QMessageBox::warning(NULL, "Map Editor", "Checksum error.");
	    return false;
	}

        QDataStream ds(cdata);
        ds.setByteOrder(QDataStream::LittleEndian);

        ds >> _mapVersion;
        if(_mapVersion < FH2ENGINE_LAST_VERSION)
        {
	    QApplication::restoreOverrideCursor();
	    QMessageBox::warning(NULL, "Map Editor", "Unsupported map format.");
	    return false;
        }

	int dataSize; ds >> dataSize;
	int offset = sizeof(_mapVersion) + sizeof(dataSize);
	bdata = qUncompress(reinterpret_cast<const uchar*>(cdata.data()) + offset, cdata.size() - offset);

	if(0 == bdata.size())
	{
	    QApplication::restoreOverrideCursor();
	    QMessageBox::warning(NULL, "Map Editor", "Incorrect data.");
	    return false;
	}

	if(dom.setContent(bdata))
	{
	    edata = dom.firstChildElement("data");
	    edata >> *this;
	}
	else
	{
	    QApplication::restoreOverrideCursor();
	    QMessageBox::warning(NULL, "Map Editor", "Unknown error.");
	    return false;
	}
    }
    else
    {
	edata >> *this;
    }

    return true;
}

bool MapData::saveMapXML(const QString & mapFile, bool compress) const
{
    QFile file(mapFile);

    if(! file.open(QIODevice::WriteOnly))
	return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QDomDocument doc;
    QDomElement emap = doc.createElement("map");
    doc.appendChild(emap);

    QDomElement eheader = doc.createElement("header");
    emap.appendChild(eheader);
    eheader << mapHeader;

    if(compress)
    {
	QByteArray cdata;
	quint16 checksum = 0;

	QByteArray bdata;
	bdata.reserve(size().width() * size().height() * 1024);
	QTextStream ts(&bdata);
	QDomElement edata0 = doc.createElement("data");
        edata0 << *this;
        edata0.save(ts, 4);

	QDataStream ds(&cdata, QIODevice::WriteOnly);
	ds.setByteOrder(QDataStream::LittleEndian);
	ds << static_cast<int>(FH2ENGINE_CURRENT_VERSION) << bdata.size();

	cdata.append(qCompress(bdata, 7));
	checksum = qChecksum(cdata.data(), cdata.size());

	QDomElement edata = doc.createElement("data");
	edata.setAttribute("compress", checksum);
	emap.appendChild(edata);
	edata.appendChild(doc.createTextNode(cdata.toBase64()));
    }
    else
    {
	QDomElement edata = doc.createElement("data");
        edata << *this;
	emap.appendChild(edata);
    }

    doc.insertBefore(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""), doc.firstChild());

    QTextStream out(&file);
    out.setCodec(QTextCodec::codecForName("UTF-8"));
    doc.save(out, 5, QDomNode::EncodingFromTextStream);

    QApplication::restoreOverrideCursor();

    return true;
}

QPoint MapData::mapToPoint(const QPoint & pt) const
{
    const MapTile* tile = qgraphicsitem_cast<const MapTile*>(itemAt(pt, QTransform()));
    return tile ? tile->mapPos() : QPoint(-1, -1);
}

QRect MapData::mapToRect(const QRect & rt) const
{
    return QRect(mapToPoint(rt.topLeft()), mapToPoint(rt.bottomRight()));
}

bool MP2Format::loadMap(const QString & mapFile)
{
    H2::File map(mapFile);

    if(map.open(QIODevice::ReadOnly))
    {
	// 4 byte: orig ver
	if(map.readLE32() != 0x0000005C)
	{
	    qDebug() << "Incorrect map file: " << mapFile;
    	    map.close();
    	    return false;
	}

	// difficulty: 0: easy, 1: normal, 2: hard, 3: expert
	difficulty = map.readLE16();

	// width, height
	size.setWidth(map.readByte());
	size.setHeight(map.readByte());

	// kingdom color: blue, gree, red, yellow, orange, purple
	for(int ii = 0; ii < 6; ++ii)
	    kingdomColor[ii] = map.readByte();

	// allow human: blue, gree, red, yellow, orange, purple
	for(int ii = 0; ii < 6; ++ii)
	    humanAllow[ii] = map.readByte();

	// allow comp: blue, gree, red, yellow, orange, purple
	for(int ii = 0; ii < 6; ++ii)
	    compAllow[ii] = map.readByte();

	// wins
	map.seek(0x1D);
	conditionWins = map.readByte();

	// data wins
	conditionWinsData1 = map.readByte();
	conditionWinsData2 = map.readByte();
	conditionWinsData3 = map.readLE16();
	map.seek(0x2C);
	conditionWinsData4 = map.readLE16();

	// loss
	map.seek(0x22);
	conditionLoss = map.readByte();
	// data loss
	conditionLossData1 = map.readLE16();
	map.seek(0x2E);
	conditionLossData2 = map.readLE16();

	// start with hero
	map.seek(0x25);
	startWithHero = (0 == map.readByte());

	// race color
	for(int ii = 0; ii < 6; ++ii)
	    raceColor[ii] = map.readByte();

	// name
	map.seek(0x3A);
	name = map.readString(16);

	// description
	map.seek(0x76);
	description = map.readString(143);

	// data map: width, heigth
	map.seek(0x01A4);
	if(static_cast<int>(map.readLE32()) != size.width())
	{
	    qDebug() << "MP2Format::loadMap:" << "incorrect size";
    	    return false;
	}
	if(static_cast<int>(map.readLE32()) != size.height())
	{
	    qDebug() << "MP2Format::loadMap:" << "incorrect size";
    	    return false;
	}

	// data map: mp2tile, part1
	// count blocks: width * heigth
	tiles.resize(size.width() * size.height());

	for(QVector<mp2til_t>::iterator
	    it = tiles.begin(); it != tiles.end(); ++it)
	{
	    QDataStream ds(map.read(20));
	    ds >> (*it);
	}

	// data map: mp2ext, part2
	// count blocks: 4 byte
	sprites.resize(map.readLE32());

	for(QVector<mp2ext_t>::iterator
	    it = sprites.begin(); it != sprites.end(); ++it)
	{
	    QDataStream ds(map.read(15));
	    ds >> (*it);
	}

	// cood castles
	QVector<mp2pos_t> townPosBlocks;
	townPosBlocks.reserve(72);

	// 72 x 3 byte (px, py, id)
	for(int ii = 0; ii < 72; ++ii)
	{
	    mp2pos_t twn;

	    twn.posx = map.readByte();
	    twn.posy = map.readByte();
	    twn.type = map.readByte();

	    if(0xFF != twn.posx && 0xFF != twn.posy)
		townPosBlocks.push_back(twn);
	}

	// cood resource kingdoms
	QVector<mp2pos_t> resourcePosBlocks;
	resourcePosBlocks.reserve(144);

	// 144 x 3 byte (px, py, id)
	for(int ii = 0; ii < 144; ++ii)
	{
	    mp2pos_t res;

	    res.posx = map.readByte();
	    res.posy = map.readByte();
	    res.type = map.readByte();

	    if(0xFF != res.posx && 0xFF != res.posy)
		resourcePosBlocks.push_back(res);
	}

	// byte: numObelisks
	map.readByte();

	// find count latest blocks: unknown byte ?? ?? ?? LO HI 00 00
	int blocksCount = 0;

	while(1)
	{
    	    quint8 lo = map.readByte();
    	    quint8 hi = map.readByte();

    	    if(0 == hi && 0 == lo)
		break;
    	    else
    		blocksCount = 256 * hi + lo - 1;
	}

	// read latest blocks
	for(int ii = 0; ii < blocksCount; ++ii)
	{
	    // parse block
	    QByteArray block = map.readBlock(map.readLE16());
	    QDataStream data(block);
	    data.setByteOrder(QDataStream::LittleEndian);
	    const QPoint posBlock = positionExtBlockFromNumber(ii + 1);
	    const int posMapIndex = posBlock.x() < 0 ? -1 : size.width() * posBlock.y() + posBlock.x();

	    if(0 <= posMapIndex && posMapIndex < tiles.size())
	    {
		switch(tiles[posMapIndex].objectID)
		{
		    case 0x82: // sign,bottle block: 10 byte
		    case 0xDD:
			if(10 <= block.size() && 0x01 == block.at(0))
			{
			    mp2sign_t sign; data >> sign;
			    signs.push_back(H2::SignPos(sign, posBlock));
			}
			break;

		    case 0x93: // map event block: 50 byte
			if(50 <= block.size() && 0x01 == block.at(0))
			{
			    mp2mapevent_t event; data >> event;
			    mapEvents.push_back(H2::EventPos(event, posBlock));
			}
			break;

		    case 0xA3: // castle, rnd town, rnd castle block: 70 byte
		    case 0xB0:
		    case 0xB1:
			if(block.size() == 70)
			{
			    mp2town_t castle; data >> castle;
			    castles.push_back(H2::TownPos(castle, posBlock));
			}
			break;

		    case 0xB7: // hero, jail block: 76 byte
		    case 0xFB:
			if(block.size() == 76)
			{
			    mp2hero_t hero; data >> hero;
			    heroes.push_back(H2::HeroPos(hero, posBlock));
			}
			break;

		    case 0xCF: // sphinx block: 138 byte
			if(138 <= block.size() && 0 == block.at(0))
			{
			    mp2sphinx_t sphinx; data >> sphinx;
			    sphinxes.push_back(H2::SphinxPos(sphinx, posBlock));
			}
			break;

		    default: break;
		}
	    }
	    else
	    if(block.at(0) == 0)
	    {
		// day event block: 50 byte
		if(50 <= block.size() && 0x01 == block.at(42))
		{
		    mp2dayevent_t event; data >> event;
		    dayEvents.push_back(event);
		}
		else
		// rumor block: 9 byte
		if(9 <= block.size())
		{
		    mp2rumor_t rumor; data >> rumor;
		    if(rumor.text.isEmpty())
    			qDebug() << "MP2Format::loadMap:" <<"skip empty rumor, block: " << ii;
		    else
			rumors.push_back(rumor);
		}
    		else
		    qCritical() << "MP2Format::loadMap:" <<"unknown block: " << ii << ", size: " << block.size();
	    }
	    else
	     qCritical() << "MP2Format::loadMap:" <<"unknown block: " << ii << ", size: " << block.size() << ", byte: " << block[0];
	}

	uniq = map.readLE32();
	map.close();

	//
	return true;
    }

    return false;
}

QPoint MP2Format::positionExtBlockFromNumber(int num) const
{
    for(int yy = 0; yy < size.height(); ++yy)
    {
        for(int xx = 0; xx < size.width(); ++xx)
        {
            const mp2til_t & mp2 = tiles[xx + yy * size.width()];

	    switch(mp2.objectID)
	    {
		case 0x82: // sign, bottle block
		case 0xDD:
		case 0x93: // map event block
		case 0xA3: // castle, rnd town, rnd castle block
		case 0xB0:
		case 0xB1:
		case 0xB7: // hero, jail block
		case 0xFB:
		case 0xCF: // sphinx block
		    break;

		default: continue;
	    }

            quint16 orders = mp2.quantity2;
            orders <<= 8;
            orders |= mp2.quantity1;

            if(orders && !(orders % 0x08) && (num == orders / 0x08))
		return QPoint(xx, yy);
        }
    }

    return QPoint(-1, -1);
}

QDomElement & operator<< (QDomElement & emap, const MapData & data)
{
    DefaultValues defs;
    QDomDocument doc = emap.ownerDocument();

    QDomElement eheader = doc.createElement("header");
    emap.appendChild(eheader);
    eheader << data.mapHeader;

    QDomElement erumors = doc.createElement("rumors");
    emap.appendChild(erumors);
    erumors << data.tavernRumors;

    QDomElement events = doc.createElement("events");
    emap.appendChild(events);
    events << data.mapDayEvents;

    emap << data.mapArea;

    QDomElement edefaults = doc.createElement("defaults");
    emap.appendChild(edefaults);
    edefaults << defs;

    return emap;
}

QDomElement & operator>> (QDomElement & emap, MapData & data)
{
    DefaultValues defs;

    QDomElement eheader = emap.firstChildElement("header");
    eheader >> data.mapHeader;

    QDomElement erumors = emap.firstChildElement("rumors");
    erumors >> data.tavernRumors;

    QDomElement events = emap.firstChildElement("events");
    events >> data.mapDayEvents;

    emap >> data.mapArea;

    QDomElement edefaults = emap.firstChildElement("defaults");
    edefaults >> defs;

    return emap;
}

void MapData::selectObjectImage(void)
{
    Form::SelectImageObject form;

    if(QDialog::Accepted == form.exec())
    {
        currentObject = CompositeObjectCursor(form.result);
	update();
    }
}

void MapData::showMapOptions(void)
{
    Form::MapOptions form(*this);

    if(QDialog::Accepted == form.exec())
    {
	// tab1
	mapHeader.mapName = form.lineEditName->text();
	mapHeader.mapDescription = form.plainTextEditDescription->toPlainText();
	mapHeader.mapDifficulty = qvariant_cast<int>(comboBoxCurrentData(form.comboBoxDifficulty));

	// tab2
	mapHeader.mapConditionWins.set(qvariant_cast<int>(comboBoxCurrentData(form.comboBoxWinsCond)), comboBoxCurrentData(form.comboBoxWinsCondExt));
	mapHeader.mapConditionWins.setAllowNormalVictory(form.checkBoxAllowNormalVictory->isChecked());
	mapHeader.mapConditionWins.setCompAlsoWins(form.checkBoxCompAlsoWins->isChecked());
	mapHeader.mapConditionLoss.set(qvariant_cast<int>(comboBoxCurrentData(form.comboBoxLossCond)), comboBoxCurrentData(form.comboBoxLossCondExt));
	mapHeader.mapStartWithHero = form.checkBoxStartWithHero->isChecked();

	mapHeader.mapCompColors = 0;
	mapHeader.mapHumanColors = 0;

	for(QVector<Form::PlayerStatus*>::const_iterator
    	    it = form.labelPlayers.begin(); it != form.labelPlayers.end(); ++it)
	{
	    if(mapHeader.mapKingdomColors & (*it)->color())
	    {
		/* 0: n/a, 1: human only, 2: comp only, 3: comp or human */
		if(0x01 & (*it)->status())
		    mapHeader.mapHumanColors |= (*it)->color();
		if(0x02 & (*it)->status())
		    mapHeader.mapCompColors |= (*it)->color();
	    }
	}

	// tab3
	tavernRumors = form.listWidgetRumors->results();
        mapDayEvents = form.listWidgetEvents->results();

	// tab4
	mapHeader.mapAuthors = form.plainTextEditAuthors->toPlainText();
        mapHeader.mapLicense = form.plainTextEditLicense->toPlainText();

        emit dataModified();
    }
}

const DayEvents & MapData::dayEvents(void) const
{
    return mapDayEvents;
}

void MapData::editObjectEvents(void)
{
    if(tileOverMouse)
	editOtherMapEventsDialog(*tileOverMouse);
}

void MapData::editObjectAttributes(void)
{
    if(tileOverMouse)
	switch(tileOverMouse->object())
    {
    	case MapObj::Event:	editMapEventDialog(*tileOverMouse); break;
    	case MapObj::RndCastle:
    	case MapObj::RndTown:
    	case MapObj::Castle:	editTownDialog(*tileOverMouse); break;
    	case MapObj::Bottle:
    	case MapObj::Sign:	editSignDialog(*tileOverMouse); break;
    	case MapObj::Jail:
    	case MapObj::Heroes:	editHeroDialog(*tileOverMouse); break;
	case MapObj::Sphinx:	editSphinxDialog(*tileOverMouse); break;

	case MapObj::RndMonster:
	case MapObj::RndMonster1:
	case MapObj::RndMonster2:
	case MapObj::RndMonster3:
	case MapObj::RndMonster4:
	case MapObj::Monster:	editMonsterDialog(*tileOverMouse); break;
	case MapObj::Resource:	editResourceDialog(*tileOverMouse); break;
	case MapObj::RndArtifact:
	case MapObj::RndArtifact1:
	case MapObj::RndArtifact2:
	case MapObj::RndArtifact3:
	case MapObj::Artifact:	editArtifactDialog(*tileOverMouse); break;

    	default:		break;
    }
}

void MapData::editOtherMapEventsDialog(const QPoint & pt)
{
    const MapTile* tile = mapTiles.tile(pt);
    if(tile) editOtherMapEventsDialog(*tile);
}

void MapData::editMonsterDialog(const MapTile & tile)
{
    MapMonster* monster = dynamic_cast<MapMonster*>(mapObjects.find(tile.mapPos()).data());

    if(monster)
    {
	Form::MapMonsterDialog form(*monster);

	if(QDialog::Accepted == form.exec())
	{
	    QPair<int, int> /* join condition, count */ res = form.result();
	    monster->condition = res.first;
	    monster->count = res.second;
	    emit dataModified();
	}
    }
}

void MapData::editResourceDialog(const MapTile & tile)
{
    MapResource* resource = dynamic_cast<MapResource*>(mapObjects.find(tile.mapPos()).data());

    if(resource)
    {
	Form::MapResourceDialog form(*resource);

	if(QDialog::Accepted == form.exec())
	{
	    resource->count = form.result();
	    emit dataModified();
	}
    }
}

void MapData::editArtifactDialog(const MapTile & tile)
{
    MapArtifact* artifact = dynamic_cast<MapArtifact*>(mapObjects.find(tile.mapPos()).data());

    if(artifact)
    {
	Form::MapArtifactDialog form(*artifact);

	if(QDialog::Accepted == form.exec())
	{
	    QPair<int, int> /* pickup condition, spell */ res = form.result();
	    artifact->condition = res.first;
	    artifact->spell = res.second;
	    emit dataModified();
	}
    }
}

void MapData::editOtherMapEventsDialog(const MapTile & tile)
{
    MapActions* actions = dynamic_cast<MapActions*>(mapObjects.find(tile.mapPos(), true).data());

    Form::ObjectEventsDialog form(actions);

    if(QDialog::Accepted == form.exec())
    {
	if(!actions)
	{
	    actions = new MapActions(tile.mapPos(), tile.levels1Const().topSpriteUID());
	    mapObjects.push_back(actions);
	}

	actions->list = form.results();

	emit dataModified();
    }
}

void MapData::editMapEventDialog(const MapTile & tile)
{
    MapEvent* event = dynamic_cast<MapEvent*>(mapObjects.find(tile.mapPos()).data());

    if(event)
    {
	Form::MapEventDialog form(*event, mapHeader.mapKingdomColors);

	if(QDialog::Accepted == form.exec())
	{
	    *event = form.result(event->pos(), event->uid());

	    emit dataModified();
	}
    }
}

void MapData::editTownDialog(const QPoint & pt)
{
    const MapTile* tile = mapTiles.tile(pt);
    if(tile) editTownDialog(*tile);
}

void MapData::editTownDialog(const MapTile & tile)
{
    MapTown* town = dynamic_cast<MapTown*>(mapObjects.find(tile.mapPos()).data());

    if(town)
    {
	Form::MapTownDialog form(*town);

	if(QDialog::Accepted == form.exec())
	{
	    town->nameTown = form.comboBoxName->lineEdit()->text();
	    town->buildings = form.checkBoxBuildingsDefault->isChecked() ? 0 : form.buildings();
	    town->dwellings = form.checkBoxDwellingsDefault->isChecked() ? 0 : form.dwellings();
	    town->troops = form.troops();
	    town->forceTown = ! form.checkBoxAllowCastle->isChecked();
	    town->captainPresent = form.checkBoxCaptain->isChecked();
	    town->customTroops = ! form.checkBoxTroopsDefault->isChecked();
	    town->customBuildings = ! form.checkBoxBuildingsDefault->isChecked();
	    town->customDwellings = ! form.checkBoxDwellingsDefault->isChecked();
	    town->col = qvariant_cast<int>(comboBoxCurrentData(form.comboBoxColor));
	    //town->race = ;

	    updateTownRaceColor(tile, town->race, town->col);
	    updateKingdomColors(town->col);
	    updatePlayersRaces();
	    emit dataModified();
	}
    }
}

MapTile* MapData::itemAtAsTile(const QPointF & pos)
{
    QList<QGraphicsItem*> tileItems = items(pos, Qt::ContainsItemShape, Qt::AscendingOrder);
    return tileItems.size() ? qgraphicsitem_cast<MapTile*>(tileItems.front()) : NULL;
}

QGraphicsPixmapItem* MapData::itemAtAsHero(const QPointF & pos)
{
    QList<QGraphicsItem*> tileItems = items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder);
    return 2 > tileItems.size() ? NULL : qgraphicsitem_cast<QGraphicsPixmapItem*>(tileItems.front());
}

void MapData::addHeroItem(const QPoint & mpos, const MapTileExt & ext)
{
    const QSize & tileSize = EditorTheme::tileSize();
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem();
    item->setData(0, GraphicsPixmapItemType::Hero);
    item->setOffset(mpos.x() * tileSize.width(), mpos.y() * tileSize.height() - 15);
    item->setPixmap(EditorTheme::getImageICN(ext.icn(), ext.index()).first);
    addItem(item);
}

void MapData::removeHeroItem(const MapTile & tile)
{
    // remove hero sprite
    QList<QGraphicsItem*> itemsList = items(tile.boundingRect(), Qt::IntersectsItemShape, Qt::AscendingOrder);

    for(QList<QGraphicsItem*>::iterator
	it = itemsList.begin(); it != itemsList.end(); ++it)
	if(GraphicsPixmapItemType::Hero == (*it)->data(0)) removeItem(*it);
}

void MapData::editHeroDialog(const QPoint & pt)
{
    const MapTile* tile = mapTiles.tile(pt);
    if(tile) editHeroDialog(*tile);
}

void MapData::editHeroDialog(const MapTile & tile)
{
    MapHero* hero = dynamic_cast<MapHero*>(mapObjects.find(tile.mapPos()).data());

    if(hero)
    {
	Form::MapHeroDialog form(*hero);

	if(QDialog::Accepted == form.exec())
	{
	    hero->attack = form.spinBoxAttack->value();
	    hero->defence = form.spinBoxDefence->value();
	    hero->power = form.spinBoxPower->value();
	    hero->knowledge = form.spinBoxKnowledge->value();
	    hero->nameHero = form.lineEditName->text();
	    hero->troops = form.troops();
	    hero->portrait = form.verticalScrollBarPort->value();
	    hero->artifacts = form.artifacts();
	    hero->experience = form.lineEditExperience->text().toInt();
	    hero->patrolMode = form.checkBoxEnablePatrol->isChecked();
	    hero->patrolSquare = qvariant_cast<int>(comboBoxCurrentData(form.comboBoxPatrol));
	    hero->skills = form.skills();
	    hero->magicBook = form.book();
	    hero->spells = form.spells();
	    hero->col = qvariant_cast<int>(comboBoxCurrentData(form.comboBoxColor));
	    hero->race = qvariant_cast<int>(comboBoxCurrentData(form.comboBoxRace));

	    updateHeroRaceColor(tile, hero->race, hero->col);
	    updateKingdomColors(hero->col);
	    updatePlayersRaces();

	    QGraphicsPixmapItem* item = itemAtAsHero(tile.boundingRect().center());
	    if(item)
	    {
		const MapTileExt* ext = tile.levels1Const().findConst(MapTileExt::isMiniHero);
		if(ext) item->setPixmap(EditorTheme::getImageICN(ext->icn(), ext->index()).first);
	    }
	    emit dataModified();
	}
    }
}

void MapData::editSphinxDialog(const MapTile & tile)
{
    MapSphinx* sphinx = dynamic_cast<MapSphinx*>(mapObjects.find(tile.mapPos()).data());

    if(sphinx)
    {
	Form::MapSphinxDialog form(*sphinx);

	if(QDialog::Accepted == form.exec())
	{
	    *sphinx = form.result(sphinx->pos(), sphinx->uid());

	    emit dataModified();
	}
    }
}

void MapData::editSignDialog(const MapTile & tile)
{
    MapSign* sign = dynamic_cast<MapSign*>(mapObjects.find(tile.mapPos()).data());

    if(sign)
    {
	Form::SignDialog form(sign->message);

	if(QDialog::Accepted == form.exec())
	{
	    sign->message = form.plainTextEdit->toPlainText();

	    emit dataModified();
	}
    }
}


void MapData::addMapObject(const QPoint & pos, const CompositeObject & obj, quint32 uid)
{
    const QSize & tileSize = EditorTheme::tileSize();

    // add sprites section
    for(CompositeObject::const_iterator
	it = obj.begin(); it != obj.end(); ++it)
    {
	QPoint offset((*it).spritePos.x() * tileSize.width() + 1, (*it).spritePos.y() * tileSize.height() + 1);
	MapTile* tile = mapTiles.mapToTile(pos + offset);
	tile->addSpriteSection(*it, uid);

	// add object info
	if((*it).spriteLevel == SpriteLevel::Action)
	{
	    MapObject* objPtr = NULL;

	    switch(obj.classId)
	    {
		case MapObj::Bottle:
		case MapObj::Sign:	objPtr = new MapSign(tile->mapPos(), uid); break;
		case MapObj::Event:	objPtr = new MapEvent(tile->mapPos(), uid); break;
    		case MapObj::Sphinx:	objPtr = new MapSphinx(tile->mapPos(), uid); break;

		case MapObj::Resource:
		case MapObj::RndResource:
		{
		    const MapTileExt* ext = tile->levels1Const().findConst(MapTileExt::isResource);
		    if(ext)
			objPtr = new MapResource(tile->mapPos(), uid, MapTileExt::resource(*ext));
		}
		break;

		case MapObj::Monster:
		case MapObj::RndMonster:
		case MapObj::RndMonster1:
		case MapObj::RndMonster2:
		case MapObj::RndMonster3:
		case MapObj::RndMonster4:
		{
		    const MapTileExt* ext = tile->levels1Const().findConst(MapTileExt::isMonster);
		    if(ext)
			objPtr = new MapMonster(tile->mapPos(), uid, MapTileExt::monster(*ext));
		}
		break;

		case MapObj::Artifact:
		case MapObj::RndArtifact:
		case MapObj::RndArtifact1:
		case MapObj::RndArtifact2:
		case MapObj::RndArtifact3:
		case MapObj::RndUltimateArtifact:
		{
		    const MapTileExt* ext = tile->levels1Const().findConst(MapTileExt::isArtifact);
		    if(ext)
			objPtr = new MapArtifact(tile->mapPos(), uid, MapTileExt::artifact(*ext));
		}
		break;

    		case MapObj::Heroes:
		{
		    const MapTileExt* ext = tile->levels1Const().findConst(MapTileExt::isMiniHero);

		    if(ext)
		    {
			MapHero* hero = new MapHero(tile->mapPos(), uid);
			addHeroItem(tile->mapPos(), *ext);
			hero->updateInfo(ext->index());
			objPtr = hero;

			mapObjects.push_back(objPtr);
			//updateHeroRaceColor(*tile, hero->race, hero->col);
			updateKingdomColors(hero->col);
			updatePlayersRaces();
			objPtr = NULL;
		    }
		}
		break;

    		case MapObj::RndCastle:
    		case MapObj::RndTown:
    		case MapObj::Castle:
		{	
		    const MapTileExt* ext = tile->levels1Const().findConst(MapTileExt::isTown);

		    if(ext)
		    {
			MapTown* town = new MapTown(tile->mapPos(), uid);
			town->updateInfo(ext->index(), obj.classId != MapObj::Castle);
			objPtr = town;

			mapObjects.push_back(objPtr);
			updatePlayersRaces();
			objPtr = NULL;
		    }
		}
		break;

		default: break;
	    }

	    if(objPtr)
		mapObjects.push_back(objPtr);
	}
    }

    emit dataModified();
}

void MapData::updatePlayersRaces(void)
{
    // update player race
    mapHeader.resetPlayerRace();

    QList<SharedMapObject> listCastles = mapObjects.list(MapObj::Castle);
    for(QList<SharedMapObject>::const_iterator
        it = listCastles.begin(); it != listCastles.end(); ++it)
    {
	MapTown* town = dynamic_cast<MapTown*>((*it).data());
	if(town) mapHeader.updatePlayerRace(town->color(), town->race);
    }

    QList<SharedMapObject> listHeroes = mapObjects.list(MapObj::Heroes);
    for(QList<SharedMapObject>::const_iterator
        it = listHeroes.begin(); it != listHeroes.end(); ++it)
    {
	MapHero* hero = dynamic_cast<MapHero*>((*it).data());
	if(hero)  mapHeader.updatePlayerRace(hero->color(), hero->race);
    }
}

void MapData::updateHeroRaceColor(const MapTile & tile, int race, int color)
{
    const MapTileExt* ext = tile.levels1Const().findConst(MapTileExt::isMiniHero);

    if(ext)
    {
	MapTileExt::updateMiniHero(*const_cast<MapTileExt*>(ext), race, color);
	update(tile.boundingRect());
    }
}

void MapData::updateTownRaceColor(const MapTile & tile, int race, int color)
{
    const QPoint & mp = tile.mapPos();
    MapTile* ltile = mapTiles.tile(QPoint(mp.x() - 1, mp.y()));
    MapTile* rtile = mapTiles.tile(QPoint(mp.x() + 1, mp.y()));

    if(ltile && rtile)
    {
	MapTileExt* lext = ltile->levels1().find(MapTileExt::isFlag32);
	if(! lext) lext = ltile->levels2().find(MapTileExt::isFlag32);
	if(lext) MapTileExt::updateFlagColor(*lext, color);
	update(ltile->boundingRect());

	MapTileExt* rext = rtile->levels1().find(MapTileExt::isFlag32);
	if(! rext) rext = rtile->levels2().find(MapTileExt::isFlag32);
	if(rext) MapTileExt::updateFlagColor(*rext, color);
	update(rtile->boundingRect());
    }
}

void MapData::updateKingdomColors(int color)
{
    mapHeader.mapKingdomColors = 0;

    QList<SharedMapObject> listCastles = mapObjects.list(MapObj::Castle);
    for(QList<SharedMapObject>::const_iterator
        it = listCastles.begin(); it != listCastles.end(); ++it)
    {
	MapTown* town = dynamic_cast<MapTown*>((*it).data());
	if(town) mapHeader.mapKingdomColors |= town->color();
    }

    QList<SharedMapObject> listHeroes = mapObjects.list(MapObj::Heroes);
    for(QList<SharedMapObject>::const_iterator
        it = listHeroes.begin(); it != listHeroes.end(); ++it)
    {
	MapHero* hero = dynamic_cast<MapHero*>((*it).data());
	if(hero) mapHeader.mapKingdomColors |= hero->color();
    }

    mapHeader.mapCompColors |= color;
    mapHeader.mapHumanColors |= color;

    if(0 == mapHeader.mapHumanColors)
    {
	mapHeader.mapHumanColors = mapHeader.mapKingdomColors;
	mapHeader.mapCompColors |= mapHeader.mapKingdomColors;
    }
}

void MapData::showPassableTriggered(void)
{
    showPassable = showPassable ? false : true;
    update();
}

QPair<int, int> MapData::versions(void)
{
    return qMakePair(_mapVersion, FH2ENGINE_CURRENT_VERSION);
}

const MapTile* MapData::currentTile(void) const
{
    return tileOverMouse;
}
