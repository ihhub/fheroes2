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
#ifndef H2MAPSFILEINFO_H
#define H2MAPSFILEINFO_H

#include <vector>

#include "gamedefs.h"

namespace Maps
{
    class FileInfo
    {
    public:
        FileInfo();
        FileInfo( const FileInfo & );

        FileInfo & operator=( const FileInfo & );

        bool ReadMAP( const std::string & );
        bool ReadMP2( const std::string & );
        bool ReadSAV( const std::string & );

        bool operator==( const FileInfo & fi ) const
        {
            return file == fi.file;
        }
        static bool NameSorting( const FileInfo &, const FileInfo & );
        static bool FileSorting( const FileInfo &, const FileInfo & );
        static bool NameCompare( const FileInfo &, const FileInfo & );

        bool isAllowCountPlayers( int playerCount ) const;
        bool isMultiPlayerMap( void ) const;
        int AllowCompHumanColors( void ) const;
        int AllowComputerColors( void ) const;
        int AllowHumanColors( void ) const;
        int HumanOnlyColors( void ) const;
        int ComputerOnlyColors( void ) const;

        int KingdomRace( int color ) const;

        int ConditionWins( void ) const;
        int ConditionLoss( void ) const;
        bool WinsCompAlsoWins( void ) const;
        bool WinsAllowNormalVictory( void ) const;
        int WinsFindArtifactID( void ) const;
        bool WinsFindUltimateArtifact( void ) const;
        u32 WinsAccumulateGold( void ) const;
        Point WinsMapsPositionObject( void ) const;
        Point LossMapsPositionObject( void ) const;
        u32 LossCountDays( void ) const;

        std::string String( void ) const;
        void Reset( void );
        void FillUnions( void );

        std::string file;
        std::string name;
        std::string description;

        u16 size_w;
        u16 size_h;
        u8 difficulty;
        u8 races[KINGDOMMAX];
        u8 unions[KINGDOMMAX];

        u8 kingdom_colors;
        u8 allow_human_colors;
        u8 allow_comp_colors;
        u8 rnd_races;

        u8 conditions_wins; // 0: wins def, 1: town, 2: hero, 3: artifact, 4: side, 5: gold
        bool comp_also_wins;
        bool allow_normal_victory;
        u16 wins1;
        u16 wins2;
        u8 conditions_loss; // 0: loss def, 1: town, 2: hero, 3: out time
        u16 loss1;
        u16 loss2;

        u32 localtime;

        bool with_heroes;
    };

    StreamBase & operator<<( StreamBase &, const FileInfo & );
    StreamBase & operator>>( StreamBase &, FileInfo & );
}

typedef std::vector<Maps::FileInfo> MapsFileInfoList;

bool PrepareMapsFileInfoList( MapsFileInfoList &, bool multi );

#endif
