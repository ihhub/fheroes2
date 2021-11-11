/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include <array>
#include <ctime>
#include <string>

#include "highscores.h"
#include "zzlib.h"
#include <functional>

namespace
{
    const uint16_t hgsID1 = 0xF1F3;
    const uint16_t hgsID2 = 0xF1F4;
    const uint8_t hgsArraySizeMax = 10;

    const std::array<Monster::monster_t, 66> monstersInRanking = { Monster::PEASANT,       Monster::GOBLIN,
                                                                   Monster::SPRITE,        Monster::HALFLING,
                                                                   Monster::CENTAUR,       Monster::ROGUE,
                                                                   Monster::SKELETON,      Monster::ORC,
                                                                   Monster::ZOMBIE,        Monster::ARCHER,
                                                                   Monster::RANGER,        Monster::BOAR,
                                                                   Monster::DWARF,         Monster::MUTANT_ZOMBIE,
                                                                   Monster::ORC_CHIEF,     Monster::ELF,
                                                                   Monster::GARGOYLE,      Monster::PIKEMAN,
                                                                   Monster::GRAND_ELF,     Monster::BATTLE_DWARF,
                                                                   Monster::NOMAD,         Monster::VETERAN_PIKEMAN,
                                                                   Monster::WOLF,          Monster::MUMMY,
                                                                   Monster::IRON_GOLEM,    Monster::ROYAL_MUMMY,
                                                                   Monster::OGRE,          Monster::GRIFFIN,
                                                                   Monster::SWORDSMAN,     Monster::DRUID,
                                                                   Monster::STEEL_GOLEM,   Monster::MASTER_SWORDSMAN,
                                                                   Monster::AIR_ELEMENT,   Monster::GREATER_DRUID,
                                                                   Monster::FIRE_ELEMENT,  Monster::GHOST,
                                                                   Monster::VAMPIRE,       Monster::WATER_ELEMENT,
                                                                   Monster::EARTH_ELEMENT, Monster::ROC,
                                                                   Monster::MINOTAUR,      Monster::CAVALRY,
                                                                   Monster::TROLL,         Monster::MAGE,
                                                                   Monster::MEDUSA,        Monster::LICH,
                                                                   Monster::OGRE_LORD,     Monster::MINOTAUR_KING,
                                                                   Monster::CHAMPION,      Monster::WAR_TROLL,
                                                                   Monster::VAMPIRE_LORD,  Monster::ARCHMAGE,
                                                                   Monster::POWER_LICH,    Monster::UNICORN,
                                                                   Monster::HYDRA,         Monster::PALADIN,
                                                                   Monster::GENIE,         Monster::CRUSADER,
                                                                   Monster::CYCLOPS,       Monster::GIANT,
                                                                   Monster::PHOENIX,       Monster::BONE_DRAGON,
                                                                   Monster::GREEN_DRAGON,  Monster::RED_DRAGON,
                                                                   Monster::TITAN,         Monster::BLACK_DRAGON };

    bool RatingSort( const HighScore::HighScoreStandardData & h1, const HighScore::HighScoreStandardData & h2 )
    {
        return h1._rating > h2._rating;
    }

    bool DaysSort( const HighScore::HighScoreCampaignData & h1, const HighScore::HighScoreCampaignData & h2 )
    {
        return h1._days < h2._days;
    }

    template <typename T>
    void TrySaveScore( std::vector<T> & highScores, T & highScore, bool( scoreCompareFunction )( const T & score1, const T & score2 ) )
    {
        // duplicate score
        if ( highScores.end() != std::find( highScores.begin(), highScores.end(), highScore ) )
            return;

        highScores.emplace_back( highScore );
        std::sort( highScores.begin(), highScores.end(), scoreCompareFunction );
        if ( highScores.size() > hgsArraySizeMax )
            highScores.resize( hgsArraySizeMax );
    }
}

namespace HighScore
{
    StreamBase & operator<<( StreamBase & msg, const HighScoreStandardData & data )
    {
        return msg << data._player << data._scenarioName << data._localTime << data._days << data._rating;
    }

    StreamBase & operator>>( StreamBase & msg, HighScoreStandardData & data )
    {
        return msg >> data._player >> data._scenarioName >> data._localTime >> data._days >> data._rating;
    }

    bool HighScoreStandardData::operator==( const HighScoreStandardData & other ) const
    {
        return _player == other._player && _scenarioName == other._scenarioName && _days == other._days;
    }

    StreamBase & operator<<( StreamBase & msg, const HighScoreCampaignData & data )
    {
        return msg << data._player << data._campaignName << data._localTime << data._days;
    }

    StreamBase & operator>>( StreamBase & msg, HighScoreCampaignData & data )
    {
        return msg >> data._player >> data._campaignName >> data._localTime >> data._days;
    }

    bool HighScoreCampaignData::operator==( const HighScoreCampaignData & other ) const
    {
        return _player == other._player && _campaignName == other._campaignName && _days == other._days;
    }

    bool HighScoreDataContainer::load( const std::string & fileName )
    {
        ZStreamFile hdata;
        if ( !hdata.read( fileName ) )
            return false;

        hdata.setbigendian( true );
        u16 hgs_id = 0;

        hdata >> hgs_id;

        if ( hgs_id == hgsID1 ) {
            hdata >> _highScoresStandard;
            return !hdata.fail();
        }
        if ( hgs_id == hgsID2 ) {
            hdata >> _highScoresStandard >> _highScoresCampaign;
            return !hdata.fail();
        }

        return false;
    }

    bool HighScoreDataContainer::save( const std::string & fileName ) const
    {
        ZStreamFile hdata;
        hdata.setbigendian( true );
        hdata << hgsID2 << _highScoresStandard << _highScoresCampaign;

        return !hdata.fail() && hdata.write( fileName );
    }

    void HighScoreDataContainer::registerScoreStandard( const std::string & playerName, const std::string & scenarioName, const uint32_t days, const uint32_t rating )
    {
        HighScoreStandardData highScore;

        highScore._player = playerName;
        highScore._scenarioName = scenarioName;
        highScore._localTime = std::time( nullptr );
        highScore._days = days;
        highScore._rating = rating;

        TrySaveScore( _highScoresStandard, highScore, RatingSort );
    }

    void HighScoreDataContainer::registerScoreCampaign( const std::string & playerName, const std::string & campaignName, const uint32_t days )
    {
        HighScoreCampaignData highScore;

        highScore._player = playerName;
        highScore._campaignName = campaignName;
        highScore._localTime = std::time( nullptr );
        highScore._days = days;

        TrySaveScore( _highScoresCampaign, highScore, DaysSort );
    }

    Monster HighScoreDataContainer::getMonsterByRating( const size_t rating )
    {
        static std::vector<std::pair<size_t, Monster::monster_t>> monsterRatings;

        if ( monsterRatings.empty() ) {
            uint32_t ratingSoFar = 0;
            uint32_t ratingIncrementCount = 0;

            for ( size_t i = 0; i < monstersInRanking.size(); ++i ) {
                const Monster::monster_t monster = monstersInRanking[i];

                // 0 to 3
                if ( monster == Monster::PEASANT ) {
                    ratingIncrementCount = 3;
                }
                // 4 to 131
                else if ( monster == Monster::GOBLIN ) {
                    ratingIncrementCount = 4;
                }
                // 132 to 227
                else if ( monster == Monster::GREATER_DRUID ) {
                    ratingIncrementCount = 3;
                }
                // >= 228
                else if ( monster == Monster::BLACK_DRAGON ) {
                    ratingIncrementCount = 1;
                }

                ratingSoFar += ratingIncrementCount;
                monsterRatings.emplace_back( ratingSoFar, monstersInRanking[i] );
            }
        }

        const std::pair<size_t, Monster::monster_t> & lastData = monsterRatings.back();

        if ( rating >= lastData.first )
            return Monster( lastData.second );

        Monster::monster_t monster = Monster::PEASANT;
        for ( size_t i = 0; i < monsterRatings.size(); ++i ) {
            if ( rating <= monsterRatings[i].first ) {
                monster = monsterRatings[i].second;
                break;
            }
        }

        return Monster( monster );
    }

    Monster HighScoreDataContainer::getMonsterByDay( const size_t dayCount )
    {
        static std::vector<std::pair<size_t, Monster::monster_t>> monsterDays;

        if ( monsterDays.empty() ) {
            uint32_t daySoFar = 0;
            uint32_t dayIncrementCount = 0;

            // need int for reverse-for loop
            const int monstersInRankingCount = static_cast<int>( monstersInRanking.size() );

            for ( int i = monstersInRankingCount - 1; i >= 0; --i ) {
                const Monster::monster_t monster = monstersInRanking[i];

                // 0 to 300
                if ( monster == Monster::BLACK_DRAGON ) {
                    dayIncrementCount = 300;
                }
                // 301 to 1000
                else if ( monster == Monster::TITAN ) {
                    dayIncrementCount = 20;
                }
                // 1001 to 2000
                else if ( monster == Monster::DRUID ) {
                    dayIncrementCount = 100;
                }
                // 2001 to 5800
                else if ( monster == Monster::BATTLE_DWARF ) {
                    dayIncrementCount = 200;
                }
                // >= 5801
                else if ( monster == Monster::PEASANT ) {
                    dayIncrementCount = 1;
                }

                daySoFar += dayIncrementCount;
                monsterDays.emplace_back( daySoFar, monstersInRanking[i] );
            }
        }

        std::pair<size_t, Monster::monster_t> lastData = monsterDays.back();

        if ( dayCount >= lastData.first )
            return Monster( lastData.second );

        Monster::monster_t monster = Monster::PEASANT;
        for ( size_t i = 0; i < monsterDays.size(); ++i ) {
            if ( dayCount <= monsterDays[i].first ) {
                monster = monsterDays[i].second;
                break;
            }
        }

        return Monster( monster );
    }

    void HighScoreDataContainer::populateDefaultHighScoresStandard()
    {
        registerScoreStandard( "Lord Kilburn", "Beltway", 70, 150 );
        registerScoreStandard( "Tsabu", "Deathgate", 80, 140 );
        registerScoreStandard( "Sir Galant", "Enroth", 90, 130 );
        registerScoreStandard( "Thundax", "Lost Continent", 100, 120 );
        registerScoreStandard( "Lord Haart", "Mountain King", 120, 110 );
        registerScoreStandard( "Ariel", "Pandemonium", 140, 100 );
        registerScoreStandard( "Rebecca", "Terra Firma", 160, 90 );
        registerScoreStandard( "Sandro", "The Clearing", 180, 80 );
        registerScoreStandard( "Crodo", "Vikings!", 200, 70 );
        registerScoreStandard( "Barock", "Wastelands", 240, 60 );
    }
}
