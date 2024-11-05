/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "monster.h"

class IStreamBase;
class OStreamBase;

namespace fheroes2
{
    struct HighscoreData
    {
    public:
        HighscoreData() = default;
        HighscoreData( std::string languageAbbreviation_, std::string playerName_, std::string scenarioName_, const uint32_t completionTime_, const uint32_t dayCount_,
                       const uint32_t rating_, const uint32_t mapSeed_ )
            : languageAbbreviation( std::move( languageAbbreviation_ ) )
            , playerName( std::move( playerName_ ) )
            , scenarioName( std::move( scenarioName_ ) )
            , completionTime( completionTime_ )
            , dayCount( dayCount_ )
            , rating( rating_ )
            , mapSeed( mapSeed_ )
        {
            // Do nothing.
        }

        HighscoreData( const HighscoreData & ) = default;
        HighscoreData( HighscoreData && ) = default;

        ~HighscoreData() = default;

        HighscoreData & operator=( const HighscoreData & data ) = default;
        HighscoreData & operator=( HighscoreData && data ) noexcept = default;

        bool operator==( const HighscoreData & other ) const
        {
            // Ignore player name and completion time.
            return scenarioName == other.scenarioName && dayCount == other.dayCount && rating == other.rating && mapSeed == other.mapSeed;
        }

        std::string languageAbbreviation;
        std::string playerName;
        std::string scenarioName;
        uint32_t completionTime{ 0 };
        uint32_t dayCount{ 0 };

        // Rating is used only for standalone scenarios. Campaigns do not use this member.
        uint32_t rating{ 0 };

        // Map seed is used to identify the same game completion to avoid duplicates in highscores.
        uint32_t mapSeed{ 0 };

        static uint32_t generateCompletionTime();

        void loadV1( IStreamBase & stream );

    private:
        friend OStreamBase & operator<<( OStreamBase & stream, const HighscoreData & data );
        friend IStreamBase & operator>>( IStreamBase & stream, HighscoreData & data );
    };

    class HighScoreDataContainer
    {
    public:
        bool load( const std::string & fileName );
        bool save( const std::string & fileName ) const;

        int32_t registerScoreStandard( HighscoreData && data );
        int32_t registerScoreCampaign( HighscoreData && data );

        void populateStandardDefaultHighScores();
        void populateCampaignDefaultHighScores();

        const std::vector<HighscoreData> & getHighScoresStandard() const
        {
            return _highScoresStandard;
        }

        const std::vector<HighscoreData> & getHighScoresCampaign() const
        {
            return _highScoresCampaign;
        }

        static Monster getMonsterByRating( const size_t rating );
        static Monster getMonsterByDay( const size_t numOfDays );

        void clear()
        {
            _highScoresStandard.clear();
            _highScoresCampaign.clear();
        }

    private:
        std::vector<HighscoreData> _highScoresStandard;
        std::vector<HighscoreData> _highScoresCampaign;
    };

    OStreamBase & operator<<( OStreamBase & stream, const HighscoreData & data );
    IStreamBase & operator>>( IStreamBase & stream, HighscoreData & data );
}

#endif
