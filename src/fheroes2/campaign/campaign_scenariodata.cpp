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

#include "campaign_scenariodata.h"
#include "artifact.h"
#include "maps_fileinfo.h"
#include "monster.h"
#include "race.h"
#include "resource.h"
#include "skill.h"
#include "spell.h"
#include <cassert>

namespace Campaign
{
    bool isCampaignMap( const std::string & fullMapPath, const std::string & scenarioMapName )
    {
        return fullMapPath.find( scenarioMapName ) != std::string::npos;
    }

    bool tryGetMatchingFile( const std::string & fileName, std::string & matchingFilePath )
    {
        const std::string fileExtension = fileName.substr( fileName.rfind( '.' ) + 1 );
        const ListFiles files = Settings::GetListFiles( "maps", fileExtension );

        const auto iterator = std::find_if( files.begin(), files.end(), [&fileName]( const std::string & filePath ) { return isCampaignMap( filePath, fileName ); } );

        if ( iterator != files.end() ) {
            matchingFilePath = *iterator;
            return true;
        }

        return false;
    }

    ScenarioBonusData::ScenarioBonusData()
        : _type( 0 )
        , _subType( 0 )
        , _amount( 0 )
    {}

    ScenarioBonusData::ScenarioBonusData( uint32_t type, uint32_t subType, uint32_t amount )
        : _type( type )
        , _subType( subType )
        , _amount( amount )
    {}

    std::string ScenarioBonusData::ToString() const
    {
        const std::vector<int> useAmountTypes = {ScenarioBonusData::ARTIFACT, ScenarioBonusData::RESOURCES, ScenarioBonusData::TROOP};

        std::string objectName;

        switch ( _type ) {
        case ScenarioBonusData::ARTIFACT:
            objectName = Artifact( _subType ).GetName();
            break;
        case ScenarioBonusData::RESOURCES:
            objectName = Resource::String( _subType );
            break;
        case ScenarioBonusData::TROOP:
            objectName = Monster( _subType ).GetPluralName( _amount );
            break;
        case ScenarioBonusData::SPELL:
            objectName = Spell( _subType ).GetName();
            break;
        case ScenarioBonusData::STARTING_RACE:
            objectName = Race::String( _subType );
            break;
        case ScenarioBonusData::SKILL:
            objectName = Skill::Secondary( _subType, _amount ).GetName();
            break;
        default:
            assert( 0 ); // some new bonus?
        }

        const bool useAmount = std::find( useAmountTypes.begin(), useAmountTypes.end(), _type ) != useAmountTypes.end() && _amount > 1;
        return useAmount ? std::to_string( _amount ) + " " + objectName : objectName;
    }

    StreamBase & operator<<( StreamBase & msg, const Campaign::ScenarioBonusData & data )
    {
        return msg << data._type << data._subType << data._amount;
    }

    StreamBase & operator>>( StreamBase & msg, Campaign::ScenarioBonusData & data )
    {
        return msg >> data._type >> data._subType >> data._amount;
    }

    ScenarioData::ScenarioData( int scenarioID, const std::vector<int> & nextMaps, const std::vector<ScenarioBonusData> & bonuses, const std::string & fileName,
                                const std::string & description )
        : _scenarioID( scenarioID )
        , _nextMaps( nextMaps )
        , _bonuses( bonuses )
        , _fileName( fileName )
        , _description( description )
    {}

    bool Campaign::ScenarioData::isMapFilePresent() const
    {
        std::string matchingFilePath;
        return tryGetMatchingFile( _fileName, matchingFilePath );
    }

    Maps::FileInfo Campaign::ScenarioData::loadMap() const
    {
        std::string matchingFilePath;

        Maps::FileInfo fi;
        if ( tryGetMatchingFile( _fileName, matchingFilePath ) )
            fi.ReadMP2( matchingFilePath );

        return fi;
    }
}
