/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#ifndef H2HIGHSCORES_H
#define H2HIGHSCORES_H

#include <string>

#include "monster.h"

class StreamBase;

namespace HighScore
{
    struct HighScoreStandardData
    {
        HighScoreStandardData()
            : _player()
            , _scenarioName()
            , _localTime( 0 )
            , _days( 0 )
            , _rating( 0 )
        {}

        bool operator==( const HighScoreStandardData & other ) const;

        std::string _player;
        std::string _scenarioName;
        uint32_t _localTime;
        uint32_t _days;
        uint32_t _rating;
    };

    struct HighScoreCampaignData
    {
        HighScoreCampaignData()
            : _player()
            , _campaignName()
            , _localTime( 0 )
            , _days( 0 )
        {}

        bool operator==( const HighScoreCampaignData & other ) const;

        std::string _player;
        std::string _campaignName;
        uint32_t _localTime;
        uint32_t _days;
    };

    class HighScoreDataContainer
    {
    public:
        bool load( const std::string & fileName );
        bool save( const std::string & fileName ) const;
        void registerScoreStandard( const std::string & playerName, const std::string & scenarioName, const uint32_t days, const uint32_t rating );
        void registerScoreCampaign( const std::string & playerName, const std::string & campaignName, const uint32_t days );
        void populateDefaultHighScoresStandard();

        const std::vector<HighScoreStandardData> & getHighScoresStandard() const
        {
            return _highScoresStandard;
        }

        const std::vector<HighScoreCampaignData> & getHighScoresCampaign() const
        {
            return _highScoresCampaign;
        }

        static Monster getMonsterByRating( const size_t rating );
        static Monster getMonsterByDay( const size_t dayCount );

    private:
        std::vector<HighScoreStandardData> _highScoresStandard;
        std::vector<HighScoreCampaignData> _highScoresCampaign;
    };
}

#endif
