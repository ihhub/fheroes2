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

#ifndef _EDITOR_MAPDATA_H_
#define _EDITOR_MAPDATA_H_

#include <QVector>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>

QT_BEGIN_NAMESPACE
class QDomElement;
class QAction;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

#include "engine.h"

class MapTile;
class MapTileLevels;
class MapData;
class MapArea;
class MapWindow;
class AroundGrounds;

class MP2Format
{
public:
    bool		loadMap(const QString &);
    bool		loadTiles(const QVector<mp2til_t> &, const QVector<mp2ext_t> &);
    QPoint		positionExtBlockFromNumber(int) const;

    QSize		size;

    QString		name;
    QString		description;

    quint16		difficulty;
    quint8		kingdomColor[6];
    quint8		humanAllow[6];
    quint8		compAllow[6];
    quint8		conditionWins;
    quint8		conditionWinsData1;
    quint8		conditionWinsData2;
    quint16		conditionWinsData3;
    quint16		conditionWinsData4;
    quint8		conditionLoss;
    quint16		conditionLossData1;
    quint16		conditionLossData2;
    bool		startWithHero;
    quint8		raceColor[6];
    quint32		uniq;

    QVector<mp2til_t>	tiles;
    QVector<mp2ext_t>	sprites;

    QVector<H2::TownPos>	castles;
    QVector<H2::HeroPos>	heroes;
    QVector<H2::SignPos>	signs;
    QVector<H2::EventPos>	mapEvents;
    QVector<H2::SphinxPos>	sphinxes;
    QVector<mp2dayevent_t>	dayEvents;
    QVector<mp2rumor_t>		rumors;
};

class MapTileExt
{
    quint8		spriteICN;
    quint8		spriteExt;
    quint8		spriteIndex;
    quint8		spriteLevelInt;
    quint32		spriteUID;

    friend		QDomElement & operator<< (QDomElement &, const MapTileExt &);
    friend		QDomElement & operator>> (QDomElement &, MapTileExt &);

public:
    MapTileExt(int uid = 0) : spriteICN(0), spriteExt(0), spriteIndex(0), spriteLevelInt(0), spriteUID(uid) {}
    MapTileExt(int lv, const mp2lev_t &);
    MapTileExt(const CompositeSprite &, quint32);

    bool		operator==(const MapTileExt & ext) const { return spriteUID == ext.spriteUID; }
    bool		operator==(quint32 uid) const { return uid == spriteUID; }

    static bool		sortLevel1(const MapTileExt &, const MapTileExt &);
    static bool		sortLevel2(const MapTileExt &, const MapTileExt &);

    int			level(void) const { return spriteLevelInt; }
    quint32		uid(void) const { return spriteUID; }
    int			icn(void) const { return spriteICN; }
    int			ext(void) const { return spriteExt; }
    int			index(void) const { return spriteIndex; }

    void		setUID(quint32 uid) { spriteUID = uid; }

    static bool		isActionSprite(const MapTileExt &);
    static bool		isAnimation(const MapTileExt &);
    static bool		isMapEvent(const MapTileExt &);
    static bool		isSphinx(const MapTileExt &);
    static bool		isSign(const MapTileExt &);
    static bool		isButtle(const MapTileExt &);
    static bool		isJail(const MapTileExt &);
    static bool		isMiniHero(const MapTileExt &);
    static bool		isFlag32(const MapTileExt &);
    static bool		isTown(const MapTileExt &);
    static bool		isRandomTown(const MapTileExt &);
    static int		loyaltyObject(const MapTileExt &);
    static bool		isResource(const MapTileExt &);
    static bool		isMonster(const MapTileExt &);
    static bool		isArtifact(const MapTileExt &);
    static int		resource(const MapTileExt &);
    static int		monster(const MapTileExt &);
    static int		artifact(const MapTileExt &);

    static void		updateFlagColor(MapTileExt &, int);
    static void		updateMiniHero(MapTileExt &, int, int);
};

QDomElement & operator<< (QDomElement &, const MapTileExt &);
QDomElement & operator>> (QDomElement &, MapTileExt &);

class MapTileLevels : public QList<MapTileExt>
{
public:
    MapTileLevels() {}

    MapTileExt*		find(bool (*pf)(const MapTileExt &));
    const MapTileExt*	findConst(bool (*pf)(const MapTileExt &)) const;
    void		paint(QPainter &, const QPoint &, const QPoint &) const;
    QString		infoString(void) const;
    int			topObjectID(void) const;
    quint32		topSpriteUID(void) const;
    bool		removeSprite(quint32);
    void		changeUIDs(QMap<quint32, quint32> &);
    QSet<quint32>	uids(void) const;
};

QDomElement & operator<< (QDomElement &, const MapTileLevels &);
QDomElement & operator>> (QDomElement &, MapTileLevels &);

class MapTile : public QGraphicsPixmapItem
{
public:
    MapTile(const QPoint & = QPoint(0, 0));
    MapTile(const mp2til_t &, const QPoint &);
    MapTile(const MapTile &);

    void		setGraphicsPixmapItemValues(void);
    MapTile &		operator=(const MapTile &);

    QRectF		boundingRect(void) const;

    void		importTile(const MapTile &, QMap<quint32, quint32> &);
    void		showInfo(void) const;
    int			groundType(void) const;

    static QString	indexString(int);

    QSet<quint32>	uids(void) const;
    const QPoint &	mapPos(void) const { return mpos; }
    void		setMapPos(const QPoint & pos) { mpos = pos; }
    void		setLocalPassable(int pass) { passableLocal = pass; }

    int			basePassable(void) const { return passableBase; }
    int			localPassable(void) const { return passableLocal; }
    void		paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget* = 0);
    void		loadSpriteLevels(const mp2ext_t &);
    void		sortSpritesLevels(void);
    void		setTileSprite(int, int);
    void		addSpriteSection(const CompositeSprite &, quint32);
    void		removeSpriteSection(quint32);

    MapTileLevels &	levels1(void) { return spritesLevel1; }
    MapTileLevels &	levels2(void) { return spritesLevel2; }
    const MapTileLevels & levels1Const(void) const { return spritesLevel1; }
    const MapTileLevels & levels2Const(void) const { return spritesLevel2; }

    bool		isObjectAction(void) const;
    bool		isObjectEdit(void) const;
    int			object(void) const;

protected:
    void		updateObjectID(void);
    void		updatePassable(void);

    static void		loadSpriteLevel(MapTileLevels &, int, const mp2lev_t &);

    friend		QDomElement & operator<< (QDomElement &, const MapTile &);
    friend		QDomElement & operator>> (QDomElement &, MapTile &);

    QPoint		mpos;

    int			tileSprite;
    int			tileShape;
    int			objectID;

    MapTileLevels	spritesLevel1;
    MapTileLevels	spritesLevel2;

    quint16		passableBase;
    quint16		passableLocal;
};

QDomElement & operator<< (QDomElement &, const MapTile &);
QDomElement & operator>> (QDomElement &, MapTile &);

class MapTiles : public QVector<MapTile>
{
    QSize		msize;

    friend		QDomElement & operator<< (QDomElement &, const MapTiles &);
    friend		QDomElement & operator>> (QDomElement &, MapTiles &);

public:
    MapTiles() {}
    MapTiles(const QSize &);

    bool		importTiles(const QSize &, const QVector<mp2til_t> &, const QVector<mp2ext_t> &);
    void		importTiles(const MapTiles &, const QRect &, const QPoint &, QMap<quint32, quint32> &);
    void		generateMap(int);

    QRect		fixedRect(const QRect &, const QPoint &) const;
    const QSize &	mapSize(void) const { return msize; }
    int			indexPoint(const QPoint &) const;
    bool		isValidPoint(const QPoint &) const;

    const MapTile*	tileConst(const QPoint &) const;
    MapTile*		tile(const QPoint &);
    const MapTile*	mapToTileConst(const QPoint &) const;
    MapTile*		mapToTile(const QPoint &);

    void		removeSprites(quint32);

    const MapTile*	tileFromDirectionConst(const MapTile*, int direct) const;
    MapTile*		tileFromDirection(const MapTile*, int direct);
    const MapTile*	tileFromDirectionConst(const QPoint &, int direct) const;
    MapTile*		tileFromDirection(const QPoint &, int direct);

    QString		sizeDescription(void) const;
};

QDomElement & operator<< (QDomElement &, const MapTiles &);
QDomElement & operator>> (QDomElement &, MapTiles &);

class MapArea
{
public:
    MapTiles		tiles;
    MapObjects		objects;
    quint32		uniq;

    MapArea() : uniq(1) {}
    MapArea(const QSize & sz) : tiles(sz), uniq(1) { }

    quint32		uid(void) { return uniq++; }
    const QSize &	size(void) const { return tiles.mapSize(); }

    void		importArea(const MapArea &, const QRect &, const QPoint &);
    void		importMP2Towns(const QVector<H2::TownPos> &);
    void		importMP2Heroes(const QVector<H2::HeroPos> &);
    void		importMP2Signs(const QVector<H2::SignPos> &);
    void		importMP2MapEvents(const QVector<H2::EventPos> &);
    void		importMP2SphinxRiddles(const QVector<H2::SphinxPos> &);
};

QDomElement & operator<< (QDomElement &, const MapArea &);
QDomElement & operator>> (QDomElement &, MapArea &);

class MapHeader
{
public:
    QString		mapName;
    QString		mapDescription;
    QString		mapAuthors;
    QString		mapLicense;
    int			mapDifficulty;
    int			mapKingdomColors;
    int			mapCompColors;
    int			mapHumanColors;
    int			playersRace[6];
    bool		mapStartWithHero;
    CondWins		mapConditionWins;
    CondLoss		mapConditionLoss;
    const MapArea &	mapArea;

    MapHeader(const MapArea &);

    void		resetPlayerRace(void);
    void		updatePlayerRace(int color, int race);
};

QDomElement & operator<< (QDomElement &, const MapHeader &);
QDomElement & operator>> (QDomElement &, MapHeader &);

class MapData : public QGraphicsScene
{
    Q_OBJECT

public:
    MapData(MapWindow*);

    const QString &	name(void) const;
    const QString &	description(void) const;
    const QString &	authors(void) const;
    const QString &	license(void) const;
    const QSize &	size(void) const;
    int			difficulty(void) const;
    int			kingdomColors(void) const;
    int			computerColors(void) const;
    int			humanColors(void) const;
    bool		startWithHero(void) const;
    const CondWins &	conditionWins(void) const;
    const CondLoss &	conditionLoss(void) const;
    ListStringPos	conditionHeroList(int) const;
    ListStringPos	conditionTownList(int) const;
    ListStringPos	conditionArtifactList(void) const;
    QList<QString>	conditionSideList(void) const;
    const QStringList & tavernRumorsList(void) const;
    const DayEvents &	dayEvents(void) const;

    static QPair<int, int>
			versions(void);

    void		newMap(const QSize &, int, const QString &, int);
    bool		loadMap(const QString &);

    bool		saveMapXML(const QString &, bool) const;
    bool		isValidBuffer(void) const;
    bool		showPassableMode(void) const { return showPassable; }

    QPoint		mapToPoint(const QPoint &) const;
    QRect		mapToRect(const QRect &) const;

    const MapTiles &	tiles(void) const { return mapTiles; }
    const MapObjects &	objects(void) const { return mapObjects; }

    void		editTownDialog(const QPoint &);
    void		editHeroDialog(const QPoint &);
    void		editOtherMapEventsDialog(const QPoint &);

    const MapTile*	currentTile(void) const;

signals:
    void		dataModified(void);
    void		currentTilePosChanged(const MapTile*);

public slots:
    void		showPassableTriggered(void);
    void		showMapOptions(void);

protected slots:
    void		selectAllTiles(void);
    void		editPassableDialog(void);
    void		cellInfoDialog(void);
    void		copyToBuffer(void);
    void		pasteFromBuffer(void);
    void		fillGroundAction(QAction*);
    void		removeObjectsAction(QAction*);
    void		selectObjectImage(void);
    void		editObjectAttributes(void);
    void		editObjectEvents(void);
    void		removeCurrentObject(void);

protected:
    void        	contextMenuEvent(QGraphicsSceneContextMenuEvent*);
    void                mousePressEvent(QGraphicsSceneMouseEvent*);
    void                mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void		mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
    void		drawForeground(QPainter*, const QRectF &);
    void		selectArea(QPointF, QPointF);
    void		updateKingdomColors(int);
    void		updateTownRaceColor(const MapTile &, int, int);
    void		updateHeroRaceColor(const MapTile &, int, int);
    void		updatePlayersRaces(void);

    void		addMapObject(const QPoint &, const CompositeObject &, quint32);
    void		addHeroItem(const QPoint &, const MapTileExt &);
    void		removeHeroItem(const MapTile &);

    void		editMapEventDialog(const MapTile &);
    void		editTownDialog(const MapTile &);
    void		editSignDialog(const MapTile &);
    void		editSphinxDialog(const MapTile &);
    void		editHeroDialog(const MapTile &);
    void		editMonsterDialog(const MapTile &);
    void		editResourceDialog(const MapTile &);
    void		editArtifactDialog(const MapTile &);
    void		editOtherMapEventsDialog(const MapTile &);

    bool		loadMapMP2(const QString &);
    bool		loadMapXML(const QString &);

    MapTile*		itemAtAsTile(const QPointF &);
    QGraphicsPixmapItem*
			itemAtAsHero(const QPointF &);

    friend class	MP2Format;
    friend class	MapTile;
    friend		QDomElement & operator<< (QDomElement &, const MapData &);
    friend		QDomElement & operator>> (QDomElement &, MapData &);

    MapTile*		tileOverMouse;


    MapArea		mapArea;
    MapTiles &		mapTiles;
    MapObjects &	mapObjects;
    MapHeader		mapHeader;

    DayEvents		mapDayEvents;
    TavernRumors	tavernRumors;

    CompositeObjectCursor
                        currentObject;

    QAction*            editCopyAct;
    QAction*            editPasteAct;
    QAction*            editPassableAct;
    QAction*            cellInfoAct;
    QAction*            selectAllAct;
    QAction*            addObjectAct;
    QAction*            editObjectAttrbAct;
    QAction*            editObjectEventsAct;
    QAction*            removeObjectAct;

    QActionGroup*       fillGroundAct;
    QActionGroup*       clearObjectsAct;

    bool		showPassable;
};

QDomElement & operator<< (QDomElement &, const MapData &);
QDomElement & operator>> (QDomElement &, MapData &);

QDomElement & operator<< (QDomElement &, const MapData &);
QDomElement & operator>> (QDomElement &, MapData &);

#endif
