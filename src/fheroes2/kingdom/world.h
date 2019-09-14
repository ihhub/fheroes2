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
#ifndef H2WORLD_H
#define H2WORLD_H

#include <vector>
#include <map>
#include <string>
#include "gamedefs.h"
#include "maps.h"
#include "maps_tiles.h"
#include "week.h"
#include "kingdom.h"
#include "sprite.h"
#include "castle_heroes.h"
#include "maps_objects.h"
#include "artifact_ultimate.h"

class Heroes;
class Castle;
class Kingdom;
class Recruits;
class Radar;
class MapObjectSimple;
class ActionSimple;

struct ListActions : public std::list<ActionSimple*>
{
    ~ListActions();
    void clear();
};

struct MapObjects : public std::map<u32, MapObjectSimple*>
{
    ~MapObjects();
    void		clear();
    void		add(MapObjectSimple*);
    std::list<MapObjectSimple*>
			get(const Point &);
    MapObjectSimple*	get(u32 uid);
    void		remove(const Point &);
    void		remove(u32 uid);
};

typedef std::map<s32, ListActions>	MapActions;

struct CapturedObject
{
    ObjectColor		objcol;
    Troop		guardians;
    int			split;

    CapturedObject() : split(1) {}

    int GetSplit() const { return split; }
    int GetObject() const { return objcol.first; }
    int GetColor() const { return objcol.second; }
    Troop & GetTroop() { return guardians; }

    void Set(int obj, int col) { objcol = ObjectColor(obj, col); }
    void SetColor(int col) { objcol.second = col; }
    void SetSplit(int spl) { split = spl; }

    bool GuardiansProtected() const { return guardians.isValid(); }
};

struct CapturedObjects : std::map<s32, CapturedObject>
{
    void Set(s32, int, int);
    void SetColor(s32, int);
    void ClearFog(int);
    void ResetColor(int);

    CapturedObject & Get(s32);
    Funds TributeCapturedObject(int col, int obj);

    u32	 GetCount(int, int) const;
    u32	 GetCountMines(int, int) const;
    int  GetColor(s32) const;
};

struct EventDate
{
    EventDate() : computer(false), first(0), subsequent(0), colors(0) {}

    void	LoadFromMP2(StreamBuf);

    bool        isAllow(int color, u32 date) const;
    bool        isDeprecated(u32 date) const;

    Funds       resource;
    bool        computer;
    u32         first;
    u32         subsequent;
    int         colors;
    std::string message;
};

StreamBase & operator<< (StreamBase &, const EventDate &);
StreamBase & operator>> (StreamBase &, EventDate &);

typedef std::list<std::string>		Rumors;
typedef std::list<EventDate>		EventsDate;
typedef std::vector<Maps::Tiles>	MapsTiles;


class World : protected Size
{
public:
    ~World(){ Reset(); }

    bool LoadMapMP2(const std::string &);
    bool LoadMapMAP(const std::string &);

    void NewMaps(u32, u32);

    static World &	Get();

    s32			w() const;
    s32			h() const;

    const Maps::Tiles & GetTiles(u32, u32) const;
    Maps::Tiles &	GetTiles(u32, u32);
    const Maps::Tiles & GetTiles(s32) const;
    Maps::Tiles &	GetTiles(s32);

    void		InitKingdoms();

    Kingdom &		GetKingdom(int color);
    const Kingdom &	GetKingdom(int color) const;

    const Castle*	GetCastle(const Point &) const;
    Castle*		GetCastle(const Point &);

    const Heroes*	GetHeroes(int /* hero id */) const;
    Heroes*		GetHeroes(int /* hero id */);

    const Heroes*	GetHeroes(const Point &) const;
    Heroes*		GetHeroes(const Point &);

    Heroes*		FromJailHeroes(s32);
    Heroes*		GetFreemanHeroes(int race = 0) const;

    const Heroes*	GetHeroesCondWins() const;
    const Heroes*	GetHeroesCondLoss() const;

    CastleHeroes	GetHeroes(const Castle &) const;

    const UltimateArtifact &	GetUltimateArtifact() const;
    bool			DiggingForUltimateArtifact(const Point &);

    int			GetDay() const;
    int			GetWeek() const;
    int			GetMonth() const;
    u32			CountDay() const;
    u32			CountWeek() const;
    bool		BeginWeek() const;
    bool		BeginMonth() const;
    bool		LastDay() const;
    bool		LastWeek() const;
    const Week &	GetWeekType() const;
    std::string		DateString() const;

    void		NewDay();
    void		NewWeek();
    void		NewMonth();

    const std::string & GetRumors();
    
    s32			NextTeleport(s32) const;
    MapsIndexes		GetTeleportEndPoints(s32) const;

    s32			NextWhirlpool(s32) const;
    MapsIndexes		GetWhirlpoolEndPoints(s32) const;


    void		CaptureObject(s32, int col);
    u32			CountCapturedObject(int obj, int col) const;
    u32			CountCapturedMines(int type, int col) const;
    u32			CountObeliskOnMaps();
    int			ColorCapturedObject(s32) const;
    void		ResetCapturedObjects(int);
    CapturedObject &	GetCapturedObject(s32);
    ListActions*	GetListActions(s32);

    void		ActionForMagellanMaps(int color);
    void		ActionToEyeMagi(int color) const;
    void		ClearFog(int color);
    void		UpdateRecruits(Recruits &) const;


    int 		CheckKingdomWins(const Kingdom &) const;
    bool		KingdomIsWins(const Kingdom &, int wins) const;
    int 		CheckKingdomLoss(const Kingdom &) const;
    bool		KingdomIsLoss(const Kingdom &, int loss) const;

    void		AddEventDate(const EventDate &);
    EventsDate		GetEventsDate(int color) const;

    MapEvent*		GetMapEvent(const Point &);
    MapObjectSimple*	GetMapObject(u32 uid);
    void		RemoveMapObject(const MapObjectSimple*);

    static u32		GetUniq();

    void		PostFixLoad();

private:
    World() : Size(0, 0) {};
    void		Defaults();
    void		Reset();
    void		MonthOfMonstersAction(const Monster &);
    void		PostLoad();

private:
    friend class Radar;
    friend StreamBase & operator<< (StreamBase &, const World &);
    friend StreamBase & operator>> (StreamBase &, World &);
#ifdef WITH_XML
    friend TiXmlElement & operator>> (TiXmlElement &, World &);
#endif

    MapsTiles				vec_tiles;
    AllHeroes				vec_heroes;
    AllCastles                          vec_castles;
    Kingdoms				vec_kingdoms;
    Rumors				vec_rumors;
    EventsDate                          vec_eventsday;

    // index, object, color
    CapturedObjects			map_captureobj;

    UltimateArtifact			ultimate_artifact;

    u32					day;
    u32					week;
    u32					month;

    Week				week_current;
    Week				week_next;

    int					heroes_cond_wins;
    int					heroes_cond_loss;

    MapActions				map_actions;
    MapObjects				map_objects;
};

StreamBase & operator<< (StreamBase &, const CapturedObject &);
StreamBase & operator>> (StreamBase &, CapturedObject &);
StreamBase & operator<< (StreamBase &, const World &);
StreamBase & operator>> (StreamBase &, World &);

StreamBase & operator<< (StreamBase &, const ListActions &);
StreamBase & operator>> (StreamBase &, ListActions &);

StreamBase & operator<< (StreamBase &, const MapObjects &);
StreamBase & operator>> (StreamBase &, MapObjects &);

extern World & world;

#endif
