/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "gamedefs.h"
#include "math_base.h"

class StreamBase;

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

        bool isAllowCountPlayers( int playerCount ) const;

        int AllowCompHumanColors() const
        {
            return allow_human_colors & allow_comp_colors;
        }

        int AllowHumanColors() const
        {
            return allow_human_colors;
        }

        int HumanOnlyColors() const
        {
            return allow_human_colors & ~allow_comp_colors;
        }

        int ComputerOnlyColors() const
        {
            return allow_comp_colors & ~allow_human_colors;
        }

        int KingdomRace( int color ) const;

        uint32_t ConditionWins() const;
        uint32_t ConditionLoss() const;
        bool WinsCompAlsoWins() const;
        int WinsFindArtifactID() const;

        bool WinsFindUltimateArtifact() const
        {
            return 0 == wins1;
        }

        uint32_t getWinningGoldAccumulationValue() const
        {
            return wins1 * 1000;
        }

        fheroes2::Point WinsMapsPositionObject() const
        {
            return { wins1, wins2 };
        }

        fheroes2::Point LossMapsPositionObject() const
        {
            return { loss1, loss2 };
        }

        uint32_t LossCountDays() const
        {
            return loss1;
        }

        void removeHumanColors( const int colors )
        {
            allow_human_colors &= ~colors;
        }

        std::string String() const;
        void Reset();

        std::string file;
        std::string name;
        std::string description;

        uint16_t size_w;
        uint16_t size_h;
        uint8_t difficulty;
        uint8_t races[KINGDOMMAX];
        uint8_t unions[KINGDOMMAX];

        uint8_t kingdom_colors;
        uint8_t allow_human_colors;
        uint8_t allow_comp_colors;
        uint8_t rnd_races;

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
            LOSS_EVERYTHING = 0,
            LOSS_TOWN = 1,
            LOSS_HERO = 2,
            LOSS_OUT_OF_TIME = 3
        };

        uint8_t conditions_wins; // refer to VictoryCondition
        bool comp_also_wins;
        bool allow_normal_victory;
        uint16_t wins1;
        uint16_t wins2;
        uint8_t conditions_loss; // refer to LossCondition
        uint16_t loss1;
        uint16_t loss2;

        uint32_t localtime;

        bool startWithHeroInEachCastle;

        GameVersion _version;

    private:
        void FillUnions( const int side1Colors, const int side2Colors );
    };

    StreamBase & operator<<( StreamBase &, const FileInfo & );
    StreamBase & operator>>( StreamBase &, FileInfo & );
}

using MapsFileInfoList = std::vector<Maps::FileInfo>;

namespace Maps
{
    MapsFileInfoList PrepareMapsFileInfoList( const bool multi );
}

#endif
