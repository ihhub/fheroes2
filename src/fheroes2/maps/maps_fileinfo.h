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
#include "serialize.h"

enum class GameVersion : int
{
    SUCCESSION_WARS = 0,
    PRICE_OF_LOYALTY = 1
};

namespace Maps
{
    struct FileInfo
    {
        FileInfo();
        FileInfo( const FileInfo & );

        ~FileInfo() = default;

        FileInfo & operator=( const FileInfo & );

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
        fheroes2::Point WinsMapsPositionObject( void ) const;
        fheroes2::Point LossMapsPositionObject( void ) const;
        u32 LossCountDays( void ) const;

        std::string String( void ) const;
        void Reset( void );

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

        enum VictoryCondition : uint8_t
        {
            VICTORY_DEFEAT_EVERYONE = 0,
            VICTORY_CAPTURE_TOWN = 1,
            VICTORY_KILL_HERO = 2,
            VICTORY_OBTAIN_ARTIFACT = 3,
            VICTORY_DEFEAT_OTHER_SIDE = 4,
            VICTORY_COLLECT_ENOUGH_GOLD = 5
        };

        enum LossCondition : uint8_t
        {
            LOSS_DEFAULT = 0,
            LOSS_TOWN = 1,
            LOSS_HERO = 2,
            LOSS_OUT_OF_TIME = 3
        };

        uint8_t conditions_wins; // refer to VictoryCondition
        bool comp_also_wins;
        bool allow_normal_victory;
        u16 wins1;
        u16 wins2;
        uint8_t conditions_loss; // refer to LossCondition
        u16 loss1;
        u16 loss2;

        u32 localtime;

        bool startWithHeroInEachCastle;

        GameVersion _version;

    private:
        void FillUnions( const int side1Colors, const int side2Colors );
    };

    StreamBase & operator<<( StreamBase &, const FileInfo & );
    StreamBase & operator>>( StreamBase &, FileInfo & );
}

StreamBase & operator>>( StreamBase & stream, GameVersion & version );

using MapsFileInfoList = std::vector<Maps::FileInfo>;

bool PrepareMapsFileInfoList( MapsFileInfoList &, bool multi );

#endif
