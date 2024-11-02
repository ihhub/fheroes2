/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <optional>
#include <string>
#include <vector>

#include "game_language.h"
#include "math_base.h"
#include "players.h"

class IStreamBase;
class OStreamBase;

enum class GameVersion : int
{
    SUCCESSION_WARS = 0,
    PRICE_OF_LOYALTY = 1,
    RESURRECTION = 2
};

namespace Maps
{
    namespace Map_Format
    {
        struct BaseMapFormat;
    }

    struct FileInfo
    {
    public:
        FileInfo()
        {
            Reset();
        }

        FileInfo( const FileInfo & ) = default;
        FileInfo( FileInfo && ) = default;

        ~FileInfo() = default;

        FileInfo & operator=( const FileInfo & ) = default;
        FileInfo & operator=( FileInfo && ) = default;

        bool operator==( const FileInfo & fi ) const
        {
            return filename == fi.filename;
        }

        bool readMP2Map( std::string filePath, const bool isForEditor );

        bool readResurrectionMap( std::string filePath, const bool isForEditor );

        bool loadResurrectionMap( const Map_Format::BaseMapFormat & map, std::string filePath );

        int AllowCompHumanColors() const
        {
            return colorsAvailableForHumans & colorsAvailableForComp;
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

        int WinsFindArtifactID() const
        {
            // In the original game artifact IDs start from 0 but for the victory condition it starts from 1 which aligns with fheroes2 artifact enumeration.
            return victoryConditionParams[0];
        }

        bool WinsFindUltimateArtifact() const
        {
            return victoryConditionParams[0] == 0;
        }

        uint32_t getWinningGoldAccumulationValue() const
        {
            return victoryConditionParams[0] * 1000;
        }

        fheroes2::Point WinsMapsPositionObject() const
        {
            return { victoryConditionParams[0], victoryConditionParams[1] };
        }

        fheroes2::Point LossMapsPositionObject() const
        {
            return { lossConditionParams[0], lossConditionParams[1] };
        }

        uint32_t LossCountDays() const
        {
            return lossConditionParams[0];
        }

        void removeHumanColors( const int colors )
        {
            colorsAvailableForHumans &= ~colors;
        }

        bool AllowChangeRace( const int color ) const
        {
            return ( colorsOfRandomRaces & color ) != 0;
        }

        void Reset();

        static bool sortByFileName( const FileInfo & lhs, const FileInfo & rhs );

        static bool sortByMapName( const FileInfo & lhs, const FileInfo & rhs );

        // Only Resurrection Maps contain supported language.
        std::optional<fheroes2::SupportedLanguage> getSupportedLanguage() const
        {
            if ( version == GameVersion::RESURRECTION ) {
                return mainLanguage;
            }

            return {};
        }

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

        std::string filename;
        std::string name;
        std::string description;

        uint16_t width;
        uint16_t height;
        uint8_t difficulty;

        std::array<uint8_t, maxNumOfPlayers> races;
        std::array<uint8_t, maxNumOfPlayers> unions;

        uint8_t kingdomColors;
        uint8_t colorsAvailableForHumans;
        uint8_t colorsAvailableForComp;
        uint8_t colorsOfRandomRaces;

        // Refer to the VictoryCondition enumeration.
        uint8_t victoryConditionType;
        bool compAlsoWins;
        bool allowNormalVictory;
        std::array<uint16_t, 2> victoryConditionParams;

        // Refer to the LossCondition enumeration.
        uint8_t lossConditionType;
        std::array<uint16_t, 2> lossConditionParams;

        // Timestamp of the save file, only relevant for save files
        uint32_t timestamp;

        // Only for maps made by the original Editor.
        bool startWithHeroInFirstCastle;

        GameVersion version;

        // World date at the moment the save file was created, only relevant for save files
        uint32_t worldDay;
        uint32_t worldWeek;
        uint32_t worldMonth;

        // All maps made by the original Editor are marked as supporting English only,
        // because it is unknown what language was used for these maps.
        fheroes2::SupportedLanguage mainLanguage{ fheroes2::SupportedLanguage::English };

    private:
        void FillUnions( const int side1Colors, const int side2Colors );
    };

    OStreamBase & operator<<( OStreamBase & stream, const FileInfo & fi );
    IStreamBase & operator>>( IStreamBase & stream, FileInfo & fi );
}

using MapsFileInfoList = std::vector<Maps::FileInfo>;

namespace Maps
{
    // For all map files.
    MapsFileInfoList getAllMapFileInfos( const bool isForEditor, const uint8_t humanPlayerCount );

    // Only for RESURRECTION map files.
    MapsFileInfoList getResurrectionMapFileInfos( const bool isForEditor, const uint8_t humanPlayerCount );
}

#endif
