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

#include <algorithm>
#include <array>
#include <ctime>
#include <memory>

#include "highscores.h"
#include "serialize.h"
#include "zzlib.h"

namespace
{
    const uint32_t highscoreFileMagicValue = 0xBADC0DE;
    const size_t highscoreMaximumEntries = 10;

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

    int32_t saveHighscoreEntry( fheroes2::HighscoreData && data, std::vector<fheroes2::HighscoreData> & entries, const bool isCampaign )
    {
        auto iter = std::find( entries.begin(), entries.end(), data );
        if ( iter != entries.end() ) {
            // This is the same game completion. Just replace the entry.
            *iter = std::move( data );
            return static_cast<int32_t>( iter - entries.begin() );
        }

        entries.emplace_back( data );
        std::sort( entries.begin(), entries.end(), [isCampaign]( const fheroes2::HighscoreData & first, const fheroes2::HighscoreData & second ) {
            if ( isCampaign ) {
                return first.rating < second.rating;
            }
            return first.rating > second.rating;
        } );

        if ( entries.size() > highscoreMaximumEntries ) {
            entries.resize( highscoreMaximumEntries );
        }

        iter = std::find( entries.begin(), entries.end(), data );
        if ( iter != entries.end() ) {
            return static_cast<int32_t>( iter - entries.begin() );
        }

        return -1;
    }
}

namespace fheroes2
{
    uint32_t HighscoreData::generateCompletionTime()
    {
        return static_cast<uint32_t>( std::time( nullptr ) );
    }

    StreamBase & operator<<( StreamBase & msg, const HighscoreData & data )
    {
        return msg << data.playerName << data.scenarioName << data.completionTime << data.dayCount << data.rating << data.mapSeed;
    }

    StreamBase & operator>>( StreamBase & msg, HighscoreData & data )
    {
        return msg >> data.playerName >> data.scenarioName >> data.completionTime >> data.dayCount >> data.rating >> data.mapSeed;
    }

    bool HighScoreDataContainer::load( const std::string & fileName )
    {
        ZStreamFile hdata;
        if ( !hdata.read( fileName ) ) {
            return false;
        }

        hdata.setbigendian( true );
        uint32_t magicValue = 0;

        hdata >> magicValue;

        if ( magicValue != highscoreFileMagicValue ) {
            // It is not a highscore file.
            return false;
        }

        hdata >> _highScoresStandard >> _highScoresCampaign;

        // Since the introduction of campaign difficulty we need to calculate rating of a campaign completion.
        // Before the change rating for campaigns was always 0. We need to set it to the number of days.
        for ( fheroes2::HighscoreData & data : _highScoresCampaign ) {
            if ( data.rating == 0 ) {
                data.rating = data.dayCount;
            }
        }

        if ( _highScoresStandard.size() < highscoreMaximumEntries ) {
            populateStandardDefaultHighScores();
        }
        else if ( _highScoresStandard.size() > highscoreMaximumEntries ) {
            _highScoresStandard.resize( highscoreMaximumEntries );
        }

        if ( _highScoresCampaign.size() < highscoreMaximumEntries ) {
            populateCampaignDefaultHighScores();
        }
        else if ( _highScoresCampaign.size() > highscoreMaximumEntries ) {
            _highScoresCampaign.resize( highscoreMaximumEntries );
        }

        return !hdata.fail();
    }

    bool HighScoreDataContainer::save( const std::string & fileName ) const
    {
        ZStreamFile hdata;
        hdata.setbigendian( true );
        hdata << highscoreFileMagicValue << _highScoresStandard << _highScoresCampaign;

        return !hdata.fail() && hdata.write( fileName );
    }

    int32_t HighScoreDataContainer::registerScoreStandard( HighscoreData && data )
    {
        return saveHighscoreEntry( std::move( data ), _highScoresStandard, false );
    }

    int32_t HighScoreDataContainer::registerScoreCampaign( HighscoreData && data )
    {
        return saveHighscoreEntry( std::move( data ), _highScoresCampaign, true );
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

    void HighScoreDataContainer::populateStandardDefaultHighScores()
    {
        const uint32_t currentTime = HighscoreData::generateCompletionTime();

        registerScoreStandard( { "Lord Kilburn", "Beltway", currentTime, 70, 150, 0 } );
        registerScoreStandard( { "Tsabu", "Deathgate", currentTime, 80, 140, 0 } );
        registerScoreStandard( { "Sir Galant", "Enroth", currentTime, 90, 130, 0 } );
        registerScoreStandard( { "Thundax", "Lost Continent", currentTime, 100, 120, 0 } );
        registerScoreStandard( { "Lord Haart", "Mountain King", currentTime, 120, 110, 0 } );
        registerScoreStandard( { "Ariel", "Pandemonium", currentTime, 140, 100, 0 } );
        registerScoreStandard( { "Rebecca", "Terra Firma", currentTime, 160, 90, 0 } );
        registerScoreStandard( { "Sandro", "The Clearing", currentTime, 180, 80, 0 } );
        registerScoreStandard( { "Crodo", "Vikings!", currentTime, 200, 70, 0 } );
        registerScoreStandard( { "Barock", "Wastelands", currentTime, 240, 60, 0 } );
    }

    void HighScoreDataContainer::populateCampaignDefaultHighScores()
    {
        const uint32_t currentTime = HighscoreData::generateCompletionTime();

        registerScoreCampaign( { "Antoine", "Roland", currentTime, 600, 600, 0 } );
        registerScoreCampaign( { "Astra", "Archibald", currentTime, 650, 650, 0 } );
        registerScoreCampaign( { "Agar", "Roland", currentTime, 700, 700, 0 } );
        registerScoreCampaign( { "Vatawna", "Archibald", currentTime, 750, 750, 0 } );
        registerScoreCampaign( { "Vesper", "Roland", currentTime, 800, 800, 0 } );
        registerScoreCampaign( { "Ambrose", "Archibald", currentTime, 850, 850, 0 } );
        registerScoreCampaign( { "Troyan", "Roland", currentTime, 900, 900, 0 } );
        registerScoreCampaign( { "Jojosh", "Archibald", currentTime, 1000, 1000, 0 } );
        registerScoreCampaign( { "Wrathmont", "Roland", currentTime, 2000, 2000, 0 } );
        registerScoreCampaign( { "Maximus", "Archibald", currentTime, 3000, 3000, 0 } );
    }
}
