/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#ifndef H2CAMPAIGN_SCENARIODATA_H
#define H2CAMPAIGN_SCENARIODATA_H

#include "gamedefs.h"
#include "maps_fileinfo.h"
#include "serialize.h"

namespace Campaign
{
    enum
    {
        SCENARIOICON_CLEARED = 10,
        SCENARIOICON_AVAILABLE = 11,
        SCENARIOICON_UNAVAILABLE = 12,
        SCENARIOICON_GOOD_SELECTED = 14,
        SCENARIOICON_EVIL_SELECTED = 17,
    };

    struct ScenarioBonusData
    {
    public:
        enum
        {
            RESOURCES,
            ARTIFACT,
            TROOP
        };

        uint32_t _type;
        uint32_t _subType;
        uint32_t _amount;

        ScenarioBonusData();
        ScenarioBonusData( uint32_t type, uint32_t subType, uint32_t amount );

        friend StreamBase & operator<<( StreamBase & msg, const ScenarioBonusData & data );
        friend StreamBase & operator>>( StreamBase & msg, ScenarioBonusData & data );

        std::string ToString() const;
    };

    class ScenarioData
    {
    public:
        const std::vector<int> & getNextMaps() const
        {
            return _nextMaps;
        }

        const std::vector<ScenarioBonusData> & getBonuses() const
        {
            return _bonuses;
        }

        const std::string & getFileName() const
        {
            return _fileName;
        }

        int getScenarioID() const
        {
            return _scenarioID;
        }

        const std::string & getDescription() const
        {
            return _description;
        }

        bool isMapFilePresent() const;
        const Maps::FileInfo loadMap() const;

        ScenarioData( int scenarioID, std::vector<int> nextMaps, std::vector<Campaign::ScenarioBonusData> bonuses, const std::string & fileName,
                      const std::string & description );

    private:
        int _scenarioID;
        std::vector<int> _nextMaps;
        std::vector<ScenarioBonusData> _bonuses;
        std::string _fileName;
        std::string _description; // at least for campaign maps, the description isn't obtained from the map's description, so we have to write one manually
    };
}

#endif
