/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include <array>
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
        FileInfo( const FileInfo & ) = default;
        FileInfo( FileInfo && ) = default;

        ~FileInfo() = default;

        FileInfo & operator=( const FileInfo & ) = default;
        FileInfo & operator=( FileInfo && ) = default;

        bool ReadMP2( const std::string & filePath );
        bool ReadSAV( const std::string & filePath );

        bool operator==( const FileInfo & fi ) const
        {
            return file == fi.file;
        }

        static bool NameSorting( const FileInfo & lhs, const FileInfo & rhs );
        static bool FileSorting( const FileInfo & lhs, const FileInfo & rhs );

        bool isAllowCountPlayers( int playerCount ) const;

        int AllowCompHumanColors() const
        {
            return colorsAvailableForHumans & colorsAvailableForComp;
        }

        int AllowHumanColors() const
        {
            return colorsAvailableForHumans;
        }

        int HumanOnlyColors() const
        {
            return colorsAvailableForHumans & ~colorsAvailableForComp;
        }

        int ComputerOnlyColors() const
        {
            return colorsAvailableForComp & ~colorsAvailableForHumans;
        }

        int KingdomRace( int color ) const;

        uint32_t ConditionWins() const;
        uint32_t ConditionLoss() const;
        bool WinsCompAlsoWins() const;
        int WinsFindArtifactID() const;

        bool WinsFindUltimateArtifact() const
        {
            return 0 == victoryConditionsParam1;
        }

        uint32_t getWinningGoldAccumulationValue() const
        {
            return victoryConditionsParam1 * 1000;
        }

        fheroes2::Point WinsMapsPositionObject() const
        {
            return { victoryConditionsParam1, victoryConditionsParam2 };
        }

        fheroes2::Point LossMapsPositionObject() const
        {
            return { lossConditionsParam1, lossConditionsParam2 };
        }

        uint32_t LossCountDays() const
        {
            return lossConditionsParam1;
        }

        void removeHumanColors( const int colors )
        {
            colorsAvailableForHumans &= ~colors;
        }

        std::string String() const;
        void Reset();

        std::string file;
        std::string name;
        std::string description;

        uint16_t width;
        uint16_t height;
        uint8_t difficulty;

        std::array<uint8_t, KINGDOMMAX> races;
        std::array<uint8_t, KINGDOMMAX> unions;

        uint8_t kingdomColors;
        uint8_t colorsAvailableForHumans;
        uint8_t colorsAvailableForComp;
        uint8_t colorsOfRandomRaces;

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

        // Refer to the VictoryCondition
        uint8_t victoryConditions;
        bool compAlsoWins;
        bool allowNormalVictory;
        uint16_t victoryConditionsParam1;
        uint16_t victoryConditionsParam2;

        // Refer to the LossCondition
        uint8_t lossConditions;
        uint16_t lossConditionsParam1;
        uint16_t lossConditionsParam2;

        // Timestamp of the save file, only relevant for save files
        uint32_t timestamp;

        bool startWithHeroInEachCastle;

        GameVersion version;

        // World date at the moment the save file was created, only relevant for save files
        uint32_t worldDay;
        uint32_t worldWeek;
        uint32_t worldMonth;

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
