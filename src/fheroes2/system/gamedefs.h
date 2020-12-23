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
#ifndef H2GAMEDEFS_H
#define H2GAMEDEFS_H

#include "engine.h"

#define MAJOR_VERSION 0
#define MINOR_VERSION 8
#define INTERMEDIATE_VERSION 4

#ifdef WITH_XML
#include "tinyxml.h"
#endif

#include "translations.h"
#define _( s ) Translation::gettext( s )
#define _n( a, b, c ) Translation::ngettext( a, b, c )

// hardcore defines: kingdom
#define KINGDOMMAX 6

// hardcore defines: world
#define MAXCASTLES 72
#define DAYOFWEEK 7
#define WEEKOFMONTH 4

// hardcore defines: castle
#define CASTLEMAXMONSTER 6

// hardcore defines: heroes
#define HEROESMAXARTIFACT 14
#define HEROESMAXSKILL 8
#define HEROESMAXCOUNT 71

// hardcore defines: skill
#define MAXPRIMARYSKILL 4
#define MAXSECONDARYSKILL 14

// hardcore defines: army
#define ARMYMAXTROOPS 5

// hardcore defines: interface
#define RADARWIDTH 144
#define BORDERWIDTH 16

// ai/hero speed
#define DEFAULT_SPEED_DELAY 5
#define DEFAULT_BATTLE_SPEED 4

#endif
