/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <ostream>

#include "artifact.h"
#include "artifact_info.h"
#include "battle_catapult.h"
#include "heroes_base.h"
#include "logging.h"
#include "rand.h"
#include "skill.h"

Battle::Catapult::Catapult( const HeroBase & hero, const Rand::DeterministicRandomGenerator & randomGenerator )
    : catShots( 1 )
    , doubleDamageChance( 25 )
    , canMiss( true )
    , _randomGenerator( randomGenerator )
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

uint32_t Battle::Catapult::GetDamage() const
{
    if ( doubleDamageChance == 100 || doubleDamageChance >= _randomGenerator.Get( 1, 100 ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "Catapult dealt double damage! (" << doubleDamageChance << "% chance)" )
        return 2;
    }

    return 1;
}

fheroes2::Point Battle::Catapult::GetTargetPosition( int target, bool hit )
{
    switch ( target ) {
    case CAT_WALL1:
        return hit ? fheroes2::Point( 475, 45 ) : fheroes2::Point( 495, 105 );
    case CAT_WALL2:
        return hit ? fheroes2::Point( 420, 115 ) : fheroes2::Point( 460, 175 );
    case CAT_WALL3:
        return hit ? fheroes2::Point( 415, 280 ) : fheroes2::Point( 455, 280 );
    case CAT_WALL4:
        return hit ? fheroes2::Point( 490, 390 ) : fheroes2::Point( 530, 390 );
    case CAT_TOWER1:
        return hit ? fheroes2::Point( 430, 40 ) : fheroes2::Point( 490, 120 );
    case CAT_TOWER2:
        return hit ? fheroes2::Point( 430, 300 ) : fheroes2::Point( 490, 340 );
    case CAT_BRIDGE:
        return hit ? fheroes2::Point( 400, 195 ) : fheroes2::Point( 450, 235 );
    case CAT_CENTRAL_TOWER:
        return hit ? fheroes2::Point( 580, 160 ) : fheroes2::Point( 610, 320 );
    default:
        break;
    }

    return fheroes2::Point();
}

int Battle::Catapult::GetTarget( const std::vector<uint32_t> & values ) const
{
    std::vector<uint32_t> targets;
    targets.reserve( 4 );

    // check walls
    if ( 0 != values[CAT_WALL1] )
        targets.push_back( CAT_WALL1 );
    if ( 0 != values[CAT_WALL2] )
        targets.push_back( CAT_WALL2 );
    if ( 0 != values[CAT_WALL3] )
        targets.push_back( CAT_WALL3 );
    if ( 0 != values[CAT_WALL4] )
        targets.push_back( CAT_WALL4 );

    // check right/left towers
    if ( targets.empty() ) {
        if ( values[CAT_TOWER1] )
            targets.push_back( CAT_TOWER1 );
        if ( values[CAT_TOWER2] )
            targets.push_back( CAT_TOWER2 );
    }

    // check bridge
    if ( targets.empty() ) {
        if ( values[CAT_BRIDGE] )
            targets.push_back( CAT_BRIDGE );
    }

    // check general tower
    if ( targets.empty() ) {
        if ( values[CAT_CENTRAL_TOWER] )
            targets.push_back( CAT_CENTRAL_TOWER );
    }

    if ( !targets.empty() ) {
        return _randomGenerator.Get( targets );
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "target not found.." )

    return 0;
}

bool Battle::Catapult::IsNextShotHit() const
{
    // Miss chance is 25%
    return !( canMiss && _randomGenerator.Get( 1, 20 ) < 6 );
}
