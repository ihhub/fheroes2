/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_catapult.h"
#include "artifact.h"
#include "battle_command.h"
#include "heroes_base.h"
#include "settings.h"
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

    if ( hero.HasArtifact( Artifact::BALLISTA ) )
        catShots += Artifact( Artifact::BALLISTA ).ExtraValue();
}

u32 Battle::Catapult::GetDamage() const
{
    if ( doubleDamageChance == 100 || doubleDamageChance >= Rand::Get( 1, 100 ) ) {
        DEBUG( DBG_BATTLE, DBG_TRACE, "Catapult dealt double damage! (" << doubleDamageChance << "% chance)" );
        return 2;
    }

    return 1;
}

Point Battle::Catapult::GetTargetPosition( int target )
{
    Point res;

    switch ( target ) {
    case CAT_WALL1:
        res = Point( 475, 45 );
        break;
    case CAT_WALL2:
        res = Point( 420, 115 );
        break;
    case CAT_WALL3:
        res = Point( 415, 280 );
        break;
    case CAT_WALL4:
        res = Point( 490, 390 );
        break;
    case CAT_TOWER1:
        res = Point( 430, 40 );
        break;
    case CAT_TOWER2:
        res = Point( 430, 300 );
        break;
    case CAT_CENTRAL_TOWER:
        res = Point( 580, 160 );
        break;
    case CAT_BRIDGE:
        res = Point( 400, 195 );
        break;
    case CAT_MISS:
        res = Point( 610, 320 );
        break;

    default:
        break;
    }

    return res;
}

int Battle::Catapult::GetTarget( const std::vector<u32> & values ) const
{
    std::vector<u32> targets;
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

    if ( targets.size() ) {
        // Miss chance is 25%
        return canMiss && 6 > Rand::Get( 1, 20 ) ? static_cast<int>( CAT_MISS ) : ( 1 < targets.size() ? *Rand::Get( targets ) : targets.front() );
    }

    DEBUG( DBG_BATTLE, DBG_TRACE, "target not found.." );

    return 0;
}
