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
#ifndef H2MAPS_OBJECTS_H
#define H2MAPS_OBJECTS_H

#include <vector>
#include <string>
#include "resource.h"
#include "monster.h"
#include "artifact.h"
#include "position.h"
#include "pairs.h"
#include "gamedefs.h"

class MapObjectSimple : public MapPosition
{
public:
    MapObjectSimple(int v = 0) : uid(0), type(v) {}
    virtual ~MapObjectSimple() {}

    int         GetType(void) const { return type; }
    u32         GetUID(void) const { return uid; }
    void        SetUID(u32 v) { uid = v; }

protected:
    friend StreamBase & operator<< (StreamBase &, const MapObjectSimple &);
    friend StreamBase & operator>> (StreamBase &, MapObjectSimple &);

    u32         uid;
    int         type;
};

StreamBase & operator<< (StreamBase &, const MapObjectSimple &);
StreamBase & operator>> (StreamBase &, MapObjectSimple &);

struct MapEvent : public MapObjectSimple
{
    MapEvent();

    void	LoadFromMP2(s32 index, StreamBuf);

    bool	isAllow(int color) const;
    void	SetVisited(int color);

    Funds	resources;
    Artifact	artifact;
    bool	computer;
    bool	cancel;
    int		colors;
    std::string message;
};

StreamBase & operator<< (StreamBase &, const MapEvent &);
StreamBase & operator>> (StreamBase &, MapEvent &);

typedef std::list<std::string>    RiddleAnswers;

struct MapSphinx : public MapObjectSimple
{
    MapSphinx();

    void	LoadFromMP2(s32 index, StreamBuf);

    bool	AnswerCorrect(const std::string & answer);
    void	SetQuiet(void);

    Funds		resources;
    Artifact		artifact;
    RiddleAnswers	answers;
    std::string		message;
    bool		valid;
};

StreamBase & operator<< (StreamBase &, const MapSphinx &);
StreamBase & operator>> (StreamBase &, MapSphinx &);

struct MapSign : public MapObjectSimple
{
    MapSign();
    MapSign(s32 index, const std::string &);

    void	LoadFromMP2(s32 index, StreamBuf);

    std::string		message;
};

StreamBase & operator<< (StreamBase &, const MapSign &);
StreamBase & operator>> (StreamBase &, MapSign &);

struct MapResource : public MapObjectSimple
{
    MapResource();

    ResourceCount	resource;
};

StreamBase & operator<< (StreamBase &, const MapResource &);
StreamBase & operator>> (StreamBase &, MapResource &);

struct MapArtifact : public MapObjectSimple
{
    MapArtifact();

    Artifact		artifact;
    int			condition;
    int			extended;

    Funds		QuantityFunds(void) const;
    ResourceCount	QuantityResourceCount(void) const;
};

StreamBase & operator<< (StreamBase &, const MapArtifact &);
StreamBase & operator>> (StreamBase &, MapArtifact &);

struct MapMonster : public MapObjectSimple
{
    MapMonster();

    Monster		monster;

    int			condition;
    int			count;

    Troop		QuantityTroop(void) const;
    bool		JoinConditionSkip(void) const;
    bool		JoinConditionMoney(void) const;
    bool		JoinConditionFree(void) const;
    bool		JoinConditionForce(void) const;
};

StreamBase & operator<< (StreamBase &, const MapMonster &);
StreamBase & operator>> (StreamBase &, MapMonster &);

#endif
