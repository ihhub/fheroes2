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

#ifndef H2HIGHSCORES_H
#define H2HIGHSCORES_H

#include <string>

#include "game_mode.h"

class StreamBase;

namespace HighScore
{
    struct HighScoreScenarioData
    {
        HighScoreScenarioData()
            : _localTime( 0 )
            , _days( 0 )
            , _rating( 0 )
        {}

        bool operator==( const HighScoreScenarioData & other ) const;

        std::string _player;
        std::string _scenarioName;
        uint32_t _localTime;
        uint32_t _days;
        uint32_t _rating;
    };

    struct HighScoreCampaignData
    {
        HighScoreCampaignData()
            : _localTime( 0 )
            , _days( 0 )
        {}

        bool operator==( const HighScoreCampaignData & other ) const;
        
        std::string _player;
        std::string _campaignName;
        uint32_t _localTime;
        uint32_t _days;
    };

    class HGSData
    {
    public:
        HGSData();

        bool Load( const std::string & fileName );
        bool Save( const std::string & fileName ) const;
        void ScoreRegistry( const std::string & playerName, const std::string & land, const uint32_t days, const uint32_t rating );
        void RedrawListStandard( int32_t ox, int32_t oy );
        void RedrawListCampaign( int32_t ox, int32_t oy );

    private:
        uint32_t _monsterAnimationFrameId;
        std::vector<HighScoreScenarioData> _highScores;
        std::vector<std::pair<size_t, Monster::monster_t>> _monsterRatings;
        std::vector<std::pair<size_t, Monster::monster_t>> _monsterDays;

        Monster getMonsterByRatingStandardGame( const size_t rating ) const
        {
            std::pair<size_t, Monster::monster_t> lastData = _monsterRatings.back();

            if ( rating >= lastData.first )
                return Monster( lastData.second );

            Monster::monster_t monster = Monster::PEASANT;
            for ( size_t i = 0; i < _monsterRatings.size(); ++i ) {
                if ( rating <= _monsterRatings[i].first ) {
                    monster = _monsterRatings[i].second;
                    break;
                }
            }

            return Monster( monster );
        }

        Monster getMonsterByRatingCampaignGame( const size_t dayCount ) const
        {
            std::pair<size_t, Monster::monster_t> lastData = _monsterDays.back();

            if ( dayCount >= lastData.first )
                return Monster( lastData.second );

            Monster::monster_t monster = Monster::PEASANT;
            for ( size_t i = 0; i < _monsterDays.size(); ++i ) {
                if ( dayCount <= _monsterDays[i].first ) {
                    monster = _monsterDays[i].second;
                    break;
                }
            }

            return Monster( monster );
        }
    };
}

#endif
