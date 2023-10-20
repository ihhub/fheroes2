/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_catapult.h"

#include <cassert>
#include <ostream>
#include <utility>
#include <vector>

#include "artifact.h"
#include "artifact_info.h"
#include "heroes_base.h"
#include "logging.h"
#include "rand.h"
#include "skill.h"

Battle::Catapult::Catapult( const HeroBase & hero )
    : catShots( 1 )
    , doubleDamageChance( 25 )
    , canMiss( true )
{
    switch ( hero.GetLevelSkill( Skill::Secondary::BALLISTICS ) ) {
    case Skill::Level::BASIC:
        doubleDamageChance = 50;
        canMiss = false;
        break;

    case Skill::Level::ADVANCED:
        doubleDamageChance = 50;
        catShots += 1;
        canMiss = false;
        break;

    case Skill::Level::EXPERT:
        doubleDamageChance = 100;
        catShots += 1;
        canMiss = false;
        break;

    default:
        break;
    }

    catShots += hero.GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::EXTRA_CATAPULT_SHOTS );
}

uint32_t Battle::Catapult::GetDamage( Rand::DeterministicRandomGenerator & randomGenerator ) const
{
    if ( doubleDamageChance == 100 || doubleDamageChance >= randomGenerator.Get( 1, 100 ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Catapult dealt double damage! (" << doubleDamageChance << "% chance)" )
        return 2;
    }

    return 1;
}

fheroes2::Point Battle::Catapult::GetTargetPosition( const CastleDefenseElement target, const bool hit )
{
    switch ( target ) {
    case CastleDefenseElement::WALL1:
        return hit ? fheroes2::Point( 475, 45 ) : fheroes2::Point( 495, 105 );
    case CastleDefenseElement::WALL2:
        return hit ? fheroes2::Point( 420, 115 ) : fheroes2::Point( 460, 175 );
    case CastleDefenseElement::WALL3:
        return hit ? fheroes2::Point( 415, 280 ) : fheroes2::Point( 455, 280 );
    case CastleDefenseElement::WALL4:
        return hit ? fheroes2::Point( 490, 390 ) : fheroes2::Point( 530, 390 );
    case CastleDefenseElement::TOWER1:
        return hit ? fheroes2::Point( 430, 40 ) : fheroes2::Point( 490, 120 );
    case CastleDefenseElement::TOWER2:
        return hit ? fheroes2::Point( 430, 300 ) : fheroes2::Point( 490, 340 );
    case CastleDefenseElement::BRIDGE:
        return hit ? fheroes2::Point( 400, 195 ) : fheroes2::Point( 450, 235 );
    case CastleDefenseElement::CENTRAL_TOWER:
        return hit ? fheroes2::Point( 580, 160 ) : fheroes2::Point( 610, 320 );
    default:
        break;
    }

    return fheroes2::Point();
}

Battle::CastleDefenseElement Battle::Catapult::GetTarget( const std::map<CastleDefenseElement, uint32_t> & stateOfCatapultTargets,
                                                          Rand::DeterministicRandomGenerator & randomGenerator )
{
    const auto checkTargetState = [&stateOfCatapultTargets]( const CastleDefenseElement target ) {
        const auto iter = stateOfCatapultTargets.find( target );
        assert( iter != stateOfCatapultTargets.end() );

        return ( iter->second > 0 );
    };

    std::vector<CastleDefenseElement> targets;
    targets.reserve( 4 );

    // Walls
    if ( checkTargetState( CastleDefenseElement::WALL1 ) ) {
        targets.push_back( CastleDefenseElement::WALL1 );
    }
    if ( checkTargetState( CastleDefenseElement::WALL2 ) ) {
        targets.push_back( CastleDefenseElement::WALL2 );
    }
    if ( checkTargetState( CastleDefenseElement::WALL3 ) ) {
        targets.push_back( CastleDefenseElement::WALL3 );
    }
    if ( checkTargetState( CastleDefenseElement::WALL4 ) ) {
        targets.push_back( CastleDefenseElement::WALL4 );
    }

    // Right/left towers
    if ( targets.empty() ) {
        if ( checkTargetState( CastleDefenseElement::TOWER1 ) ) {
            targets.push_back( CastleDefenseElement::TOWER1 );
        }
        if ( checkTargetState( CastleDefenseElement::TOWER2 ) ) {
            targets.push_back( CastleDefenseElement::TOWER2 );
        }
    }

    // Bridge
    if ( targets.empty() ) {
        if ( checkTargetState( CastleDefenseElement::BRIDGE ) ) {
            targets.push_back( CastleDefenseElement::BRIDGE );
        }
    }

    // Central tower
    if ( targets.empty() ) {
        if ( checkTargetState( CastleDefenseElement::CENTRAL_TOWER ) ) {
            targets.push_back( CastleDefenseElement::CENTRAL_TOWER );
        }
    }

    if ( !targets.empty() ) {
        return randomGenerator.Get( targets );
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "no target was found" )

    return CastleDefenseElement::NONE;
}

bool Battle::Catapult::IsNextShotHit( Rand::DeterministicRandomGenerator & randomGenerator ) const
{
    // Miss chance is 25%
    return !( canMiss && randomGenerator.Get( 1, 20 ) < 6 );
}
