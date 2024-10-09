/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include "bin_info.h"

#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <ostream>
#include <string>
#include <utility>

#include "agg.h"
#include "battle_cell.h"
#include "logging.h"
#include "monster.h"
#include "monster_info.h"
#include "serialize.h"

namespace
{
    template <typename T>
    T getValue( const uint8_t * data, const size_t base, const size_t offset = 0 )
    {
        return fheroes2::getLEValue<T>( reinterpret_cast<const char *>( data ), base, offset );
    }

    const char * GetFilename( const int monsterId )
    {
        return fheroes2::getMonsterData( monsterId ).binFileName;
    }

    class MonsterAnimCache
    {
    public:
        Bin_Info::MonsterAnimInfo getAnimInfo( const int monsterID )
        {
            auto mapIterator = _animMap.find( monsterID );
            if ( mapIterator != _animMap.end() ) {
                return mapIterator->second;
            }

            Bin_Info::MonsterAnimInfo info( monsterID, AGG::getDataFromAggFile( GetFilename( monsterID ) ) );
            if ( info.isValid() ) {
                _animMap[monsterID] = info;
                return info;
            }

            DEBUG_LOG( DBG_GAME, DBG_WARN, "Missing BIN file data: " << GetFilename( monsterID ) << ", monster ID: " << monsterID )
            return {};
        }

    private:
        std::map<int, Bin_Info::MonsterAnimInfo> _animMap;
    };

    MonsterAnimCache _infoCache;
}

namespace Bin_Info
{
    const size_t CORRECT_FRM_LENGTH = 821;

    // When base unit and its upgrade use the same FRM file (e.g. Archer and Ranger)
    // We modify animation speed value to make them go faster
    const double MOVE_SPEED_UPGRADE = 0.12;
    const double SHOOT_SPEED_UPGRADE = 0.08;
    const double RANGER_SHOOT_SPEED = 0.78;

    MonsterAnimInfo::MonsterAnimInfo( int monsterID, const std::vector<uint8_t> & bytes )
        : moveSpeed( 450 )
        , shootSpeed( 0 )
        , flightSpeed( 0 )
        , troopCountOffsetLeft( 0 )
        , troopCountOffsetRight( 0 )
        , idleAnimationCount( 0 )
        , idleAnimationDelay( 0 )
    {
        if ( bytes.size() != Bin_Info::CORRECT_FRM_LENGTH ) {
            return;
        }

        const uint8_t * data = bytes.data();

        eyePosition = { getValue<int16_t>( data, 1 ), getValue<int16_t>( data, 3 ) };

        for ( size_t moveID = 0; moveID < 7; ++moveID ) {
            std::vector<int> moveOffset;

            moveOffset.reserve( 16 );

            for ( int frame = 0; frame < 16; ++frame ) {
                moveOffset.push_back( static_cast<int>( getValue<int8_t>( data, 5 + moveID * 16, frame ) ) );
            }

            frameXOffset.emplace_back( std::move( moveOffset ) );
        }

        // Idle animations data
        idleAnimationCount = data[117];
        if ( idleAnimationCount > 5u )
            idleAnimationCount = 5u; // here we need to reset our object
        for ( uint32_t i = 0; i < idleAnimationCount; ++i )
            idlePriority.push_back( getValue<float>( data, 118, i ) );

        for ( uint32_t i = 0; i < idleAnimationCount; ++i )
            unusedIdleDelays.push_back( getValue<uint32_t>( data, 138, i ) );

        idleAnimationDelay = getValue<uint32_t>( data, 158 );

        // Monster speed data
        moveSpeed = getValue<uint32_t>( data, 162 );
        shootSpeed = getValue<uint32_t>( data, 166 );
        flightSpeed = getValue<uint32_t>( data, 170 );

        // Projectile data
        for ( size_t i = 0; i < 3; ++i ) {
            projectileOffset.emplace_back( getValue<int16_t>( data, 174 + ( i * 4 ) ), getValue<int16_t>( data, 176 + ( i * 4 ) ) );
        }

        // We change all projectile start positions to match the last projectile position in shooting animation,
        // this position will be used to calculate the projectile path, but will not be used in rendering.
        if ( monsterID == Monster::ARCHER || monsterID == Monster::RANGER ) {
            projectileOffset[0].x -= 30;
            projectileOffset[0].y += 5;
            projectileOffset[1].x -= 46;
            projectileOffset[1].y -= 5;
            projectileOffset[2].x -= 30;
            projectileOffset[2].y -= 40;
        }

        if ( monsterID == Monster::ORC || monsterID == Monster::ORC_CHIEF ) {
            for ( fheroes2::Point & offset : projectileOffset ) {
                offset.x -= 20;
                offset.y = -36;
            }
        }

        if ( monsterID == Monster::TROLL || monsterID == Monster::WAR_TROLL ) {
            projectileOffset[0].x -= 30;
            projectileOffset[0].y -= 5;
            projectileOffset[1].x -= 14;
            --projectileOffset[1].y;
            projectileOffset[2].x -= 15;
            projectileOffset[2].y -= 19;
        }

        if ( monsterID == Monster::ELF || monsterID == Monster::GRAND_ELF ) {
            projectileOffset[0].x -= 19;
            projectileOffset[0].y += 22;
            projectileOffset[1].x -= 40;
            --projectileOffset[1].y;
            projectileOffset[2].x -= 19;
            // IMPORTANT: In BIN file Elves and Grand Elves have incorrect start Y position for lower shooting attack ( -1 ).
            projectileOffset[2].y = -54;
        }

        if ( monsterID == Monster::DRUID || monsterID == Monster::GREATER_DRUID ) {
            projectileOffset[0].x -= 12;
            projectileOffset[0].y += 23;
            projectileOffset[1].x -= 20;
            projectileOffset[2].x -= 7;
            projectileOffset[2].y -= 21;
        }

        if ( monsterID == Monster::CENTAUR ) {
            projectileOffset[0].x -= 24;
            projectileOffset[0].y += 31;
            projectileOffset[1].x -= 32;
            projectileOffset[1].y += 2;
            projectileOffset[2].x -= 24;
            projectileOffset[2].y -= 27;
        }

        if ( monsterID == Monster::HALFLING ) {
            projectileOffset[0].x -= 7;
            projectileOffset[0].y += 11;
            projectileOffset[1].x -= 21;
            projectileOffset[1].y += 10;
            projectileOffset[2].x -= 9;
            projectileOffset[2].y -= 14;
        }

        if ( monsterID == Monster::MAGE || monsterID == Monster::ARCHMAGE ) {
            projectileOffset[0].y -= 2;
            projectileOffset[2].y += 2;
        }

        if ( monsterID == Monster::TITAN ) {
            projectileOffset[0].x -= 10;
            projectileOffset[0].y += 22;
            projectileOffset[1].x -= 20;
            projectileOffset[1].y += 8;
            projectileOffset[2].x -= 15;
            projectileOffset[2].y -= 22;
        }

        if ( monsterID == Monster::LICH || monsterID == Monster::POWER_LICH ) {
            projectileOffset[0].x -= 5;
            projectileOffset[0].y += 6;
            projectileOffset[1].x -= 16;
            projectileOffset[1].y -= 9;
            projectileOffset[2].x -= 9;
            projectileOffset[2].y -= 7;
        }

        uint8_t projectileCount = data[186];
        if ( projectileCount > 12u )
            projectileCount = 12u; // here we need to reset our object
        for ( uint8_t i = 0; i < projectileCount; ++i )
            projectileAngles.push_back( getValue<float>( data, 187, i ) );

        // Positional offsets for sprites & drawing
        troopCountOffsetLeft = getValue<int32_t>( data, 235 );
        troopCountOffsetRight = getValue<int32_t>( data, 239 );

        // Load animation sequences themselves
        for ( int idx = MOVE_START; idx <= SHOOT3_END; ++idx ) {
            std::vector<int> anim;
            uint8_t count = data[243 + idx];
            if ( count > 16 )
                count = 16; // here we need to reset our object
            for ( uint8_t frame = 0; frame < count; ++frame ) {
                anim.push_back( static_cast<int>( data[277 + idx * 16 + frame] ) );
            }

            animationFrames.emplace_back( std::move( anim ) );
        }

        if ( monsterID == Monster::WOLF ) { // Wolves have incorrect frame for lower attack animation
            if ( animationFrames[ATTACK3].size() == 3 && animationFrames[ATTACK3][0] == 16 ) {
                animationFrames[ATTACK3][0] = 2;
            }
            if ( animationFrames[ATTACK3_END].size() == 3 && animationFrames[ATTACK3_END][2] == 16 ) {
                animationFrames[ATTACK3_END][2] = 2;
            }
        }

        if ( monsterID == Monster::DWARF || monsterID == Monster::BATTLE_DWARF ) {
            // Dwarves and Battle Dwarves have incorrect death animation.
            if ( animationFrames[DEATH].size() == 8 ) {
                animationFrames[DEATH].clear();

                for ( int frameId = 49; frameId <= 55; ++frameId ) {
                    animationFrames[DEATH].push_back( frameId );
                }
            }
        }

        // Modify AnimInfo for upgraded monsters without own FRM file
        int speedDiff = 0;
        switch ( monsterID ) {
        case Monster::RANGER:
        case Monster::VETERAN_PIKEMAN:
        case Monster::MASTER_SWORDSMAN:
        case Monster::CHAMPION:
        case Monster::CRUSADER:
        case Monster::ORC_CHIEF:
        case Monster::OGRE_LORD:
        case Monster::WAR_TROLL:
        case Monster::BATTLE_DWARF:
        case Monster::GRAND_ELF:
        case Monster::GREATER_DRUID:
        case Monster::MINOTAUR_KING:
        case Monster::STEEL_GOLEM:
        case Monster::ARCHMAGE:
        case Monster::MUTANT_ZOMBIE:
        case Monster::ROYAL_MUMMY:
        case Monster::VAMPIRE_LORD:
        case Monster::POWER_LICH:
            speedDiff = static_cast<int>( Monster( monsterID ).GetSpeed() ) - Monster( monsterID - 1 ).GetSpeed();
            break;
        case Monster::EARTH_ELEMENT:
        case Monster::AIR_ELEMENT:
        case Monster::WATER_ELEMENT:
            speedDiff = static_cast<int>( Monster( monsterID ).GetSpeed() ) - Monster( Monster::FIRE_ELEMENT ).GetSpeed();
            break;
        default:
            break;
        }

        if ( std::abs( speedDiff ) > 0 ) {
            moveSpeed = static_cast<uint32_t>( ( 1 - MOVE_SPEED_UPGRADE * speedDiff ) * moveSpeed );
            // Ranger is special since he gets double attack on upgrade
            if ( monsterID == Monster::RANGER ) {
                shootSpeed = static_cast<uint32_t>( shootSpeed * RANGER_SHOOT_SPEED );
            }
            else {
                shootSpeed = static_cast<uint32_t>( ( 1 - SHOOT_SPEED_UPGRADE * speedDiff ) * shootSpeed );
            }
        }

        if ( frameXOffset[MOVE_STOP][0] == 0 && frameXOffset[MOVE_TILE_END][0] != 0 )
            frameXOffset[MOVE_STOP][0] = frameXOffset[MOVE_TILE_END][0];

        for ( int idx = MOVE_START; idx <= MOVE_ONE; ++idx )
            frameXOffset[idx].resize( animationFrames[idx].size(), 0 );

        if ( frameXOffset[MOVE_STOP].size() == 1 && frameXOffset[MOVE_STOP][0] == 0 ) {
            if ( frameXOffset[MOVE_TILE_END].size() == 1 && frameXOffset[MOVE_TILE_END][0] != 0 )
                frameXOffset[MOVE_STOP][0] = frameXOffset[MOVE_TILE_END][0];
            else if ( frameXOffset[MOVE_TILE_START].size() == 1 && frameXOffset[MOVE_TILE_START][0] != 0 )
                frameXOffset[MOVE_STOP][0] = 44 + frameXOffset[MOVE_TILE_START][0];
            else
                frameXOffset[MOVE_STOP][0] = frameXOffset[MOVE_MAIN].back();
        }

        // Movement animation fix for Iron and Steel Golem. Also check that the data sizes are correct.
        if ( ( monsterID == Monster::IRON_GOLEM || monsterID == Monster::STEEL_GOLEM )
             && ( frameXOffset[MOVE_START].size() == 4 && frameXOffset[MOVE_MAIN].size() == 7 && animationFrames[MOVE_MAIN].size() == 7 ) ) { // the original golem info
            frameXOffset[MOVE_START][0] = 0;
            frameXOffset[MOVE_START][1] = Battle::Cell::widthPx * 1 / 8;
            frameXOffset[MOVE_START][2] = Battle::Cell::widthPx * 2 / 8 + 3;
            frameXOffset[MOVE_START][3] = Battle::Cell::widthPx * 3 / 8;

            // 'MOVE_MAIN' animation is missing 1/4 of animation start. 'MOVE_START' (for first and one cell move) has this 1/4 of animation,
            // but 'MOVE_TILE_START` (for movement after the first cell) is empty, so we move all frames except the last frame from 'MOVE_MAIN'
            // to the end of 'MOVE_START' animationFrames. And prepare 'MOVE_TILE_START' for new animation frame IDs.
            animationFrames[MOVE_TILE_START].resize( animationFrames[MOVE_TILE_START].size() + animationFrames[MOVE_MAIN].size() - 1 );
            animationFrames[MOVE_START].insert( animationFrames[MOVE_START].end(), animationFrames[MOVE_MAIN].begin(), animationFrames[MOVE_MAIN].end() - 1 );
            animationFrames[MOVE_MAIN].erase( animationFrames[MOVE_MAIN].begin(), animationFrames[MOVE_MAIN].end() - 1 );
            // Do the same for 'x' offset vector and make a copy to 'MOVE_TILE_START'.
            frameXOffset[MOVE_START].insert( frameXOffset[MOVE_START].end(), frameXOffset[MOVE_MAIN].begin(), frameXOffset[MOVE_MAIN].end() - 1 );
            frameXOffset[MOVE_TILE_START] = frameXOffset[MOVE_MAIN];
            frameXOffset[MOVE_TILE_START].pop_back();
            frameXOffset[MOVE_MAIN].erase( frameXOffset[MOVE_MAIN].begin(), frameXOffset[MOVE_MAIN].end() - 1 );

            // Correct the 'x' offset by half of cell.
            frameXOffset[MOVE_MAIN][0] += Battle::Cell::widthPx / 2;
            for ( size_t id = 0; id < frameXOffset[MOVE_TILE_START].size(); ++id ) {
                frameXOffset[MOVE_START][id + 4] += Battle::Cell::widthPx / 2;
                // For 'MOVE_TILE_START' also include the correction, made in "agg_image.cpp".
                frameXOffset[MOVE_TILE_START][id] += Battle::Cell::widthPx / 2 - ( 6 - static_cast<int32_t>( id ) ) * Battle::Cell::widthPx / 28;
                // For 'MOVE_TILE_START' animation frames IDs use new frames, made in "agg_image.cpp", which starts from ID = 40.
                animationFrames[MOVE_TILE_START][id] = 40 + static_cast<int32_t>( id );
            }
        }

        // The Ogre has a duplicate frame of the 'STATIC' animation at the start of the 'MOVE_MAIN' animation, making him to move non-smoothly.
        if ( ( monsterID == Monster::OGRE || monsterID == Monster::OGRE_LORD ) && frameXOffset[MOVE_MAIN].size() == 9 && animationFrames[MOVE_MAIN].size() == 9 ) {
            animationFrames[MOVE_MAIN].erase( animationFrames[MOVE_MAIN].begin() );
            frameXOffset[MOVE_MAIN].erase( frameXOffset[MOVE_MAIN].begin() );
        }

        // Some creatures needs their 'x' offset in moving animations to be bigger by 3px to avoid sprite shift in Well and during diagonal movement.
        if ( monsterID == Monster::ORC || monsterID == Monster::ORC_CHIEF || monsterID == Monster::OGRE || monsterID == Monster::OGRE_LORD || monsterID == Monster::DWARF
             || monsterID == Monster::BATTLE_DWARF || monsterID == Monster::UNICORN || monsterID == Monster::CENTAUR || monsterID == Monster::BOAR
             || monsterID == Monster::LICH || monsterID == Monster::POWER_LICH || monsterID == Monster::ROGUE ) {
            for ( const int animType : { MOVE_START, MOVE_TILE_START, MOVE_MAIN, MOVE_TILE_END, MOVE_STOP, MOVE_ONE } ) {
                for ( int & xOffset : frameXOffset[animType] ) {
                    xOffset += 3;
                }
            }
        }

        // Archers/Rangers needs their 'x' offset in moving animations to be bigger by 1px to avoid sprite shift in Well and during diagonal movement.
        if ( monsterID == Monster::ARCHER || monsterID == Monster::RANGER ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                for ( int & xOffset : frameXOffset[animType] ) {
                    ++xOffset;
                }
            }
        }

        // Paladin/Crusader needs their 'x' offset in moving animations to be lower by 1px to avoid sprite shift in Well and during diagonal movement.
        if ( ( monsterID == Monster::PALADIN || monsterID == Monster::CRUSADER ) && frameXOffset[MOVE_MAIN].size() == 8 && animationFrames[MOVE_MAIN].size() == 8 ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                for ( int & xOffset : frameXOffset[animType] ) {
                    --xOffset;
                }

                // The 5th and 8th frames needs extra shift by 2 and 1 pixel.
                frameXOffset[animType][4] -= 2;
                --frameXOffset[animType][7];
            }
        }

        // Goblins needs their 'x' offset in moving animations to be bigger by 6px to avoid sprite shift in Well and during diagonal movement.
        if ( monsterID == Monster::GOBLIN ) {
            for ( const int animType : { MOVE_TILE_START, MOVE_MAIN, MOVE_STOP, MOVE_ONE } ) {
                for ( int & xOffset : frameXOffset[animType] ) {
                    xOffset += 6;
                }
            }
        }

        // Trolls needs their 'x' offset in moving animations to be bigger by 2px to avoid sprite shift in Well and during diagonal movement.
        if ( ( monsterID == Monster::TROLL || monsterID == Monster::WAR_TROLL ) && frameXOffset[MOVE_MAIN].size() == 14 && frameXOffset[MOVE_ONE].size() == 14 ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                for ( int & xOffset : frameXOffset[animType] ) {
                    xOffset += 2;
                }

                // The 7th and 14th frames needs extra shift by 1 px.
                ++frameXOffset[animType][6];
                ++frameXOffset[animType][13];
            }
        }

        // Giants/Titians needs their 'x' offset in moving animations to be corrected to avoid sprite shift in Well and during diagonal movement.
        if ( ( monsterID == Monster::GIANT || monsterID == Monster::TITAN ) && frameXOffset[MOVE_MAIN].size() == 7 && frameXOffset[MOVE_ONE].size() == 7 ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                frameXOffset[animType][0] += 3;
                frameXOffset[animType][1] += 2;
                frameXOffset[animType][2] += 2;
                ++frameXOffset[animType][5];
                ++frameXOffset[animType][6];
            }
        }

        // Nomad needs their 'x' offset in moving animations to be corrected to avoid one sprite shift in Well and during diagonal movement.
        if ( monsterID == Monster::NOMAD && frameXOffset[MOVE_MAIN].size() == 8 && frameXOffset[MOVE_ONE].size() == 8 ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                frameXOffset[animType][2] -= 2;
            }
        }

        // Nomad needs their 'x' offset in moving animations to be corrected to avoid one sprite shift in Well and during diagonal movement.
        if ( monsterID == Monster::MEDUSA && frameXOffset[MOVE_MAIN].size() == 15 && frameXOffset[MOVE_ONE].size() == 15 ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                frameXOffset[animType][3] += 2;
                frameXOffset[animType][4] += 2;
                ++frameXOffset[animType][5];
                ++frameXOffset[animType][9];
                frameXOffset[animType][10] += 2;
                ++frameXOffset[animType][11];
                ++frameXOffset[animType][14];
            }
        }

        // Elementals needs their 'x' offset in moving animations to be corrected to avoid sprite shift in Well and during diagonal movement.
        if ( ( monsterID == Monster::EARTH_ELEMENT || monsterID == Monster::AIR_ELEMENT || monsterID == Monster::FIRE_ELEMENT || monsterID == Monster::WATER_ELEMENT )
             && frameXOffset[MOVE_MAIN].size() == 8 && frameXOffset[MOVE_ONE].size() == 8 ) {
            for ( const int animType : { MOVE_MAIN, MOVE_ONE } ) {
                frameXOffset[animType][0] += 3;
                frameXOffset[animType][1] += 3;
                frameXOffset[animType][2] += 5;
                frameXOffset[animType][3] += 3;
                --frameXOffset[animType][5];
                ++frameXOffset[animType][6];
                --frameXOffset[animType][7];
            }
        }

        // X offset fix for Swordsman.
        if ( ( monsterID == Monster::SWORDSMAN || monsterID == Monster::MASTER_SWORDSMAN ) && frameXOffset[MOVE_START].size() == 2
             && frameXOffset[MOVE_STOP].size() == 1 ) {
            frameXOffset[MOVE_START][0] = 0;
            frameXOffset[MOVE_START][1] = Battle::Cell::widthPx * 1 / 8;
            for ( int & xOffset : frameXOffset[MOVE_MAIN] ) {
                xOffset += Battle::Cell::widthPx / 4 + 3;
            }

            frameXOffset[MOVE_STOP][0] = Battle::Cell::widthPx;
        }
    }

    bool MonsterAnimInfo::isValid() const
    {
        if ( animationFrames.size() != SHOOT3_END + 1 )
            return false;

        // Absolute minimal set up
        const int essentialAnimations[7] = { MOVE_MAIN, STATIC, DEATH, WINCE_UP, ATTACK1, ATTACK2, ATTACK3 };

        for ( int i = 0; i < 7; ++i ) {
            if ( animationFrames.at( essentialAnimations[i] ).empty() )
                return false;
        }

        if ( idlePriority.size() != static_cast<size_t>( idleAnimationCount ) )
            return false;

        return true;
    }

    bool MonsterAnimInfo::hasAnim( int animID ) const
    {
        return animationFrames.size() == SHOOT3_END + 1 && !animationFrames.at( animID ).empty();
    }

    size_t MonsterAnimInfo::getProjectileID( const double angle ) const
    {
        const std::vector<float> & angles = projectileAngles;
        if ( angles.empty() )
            return 0;

        for ( size_t id = 0u; id < angles.size() - 1; ++id ) {
            if ( angle >= static_cast<double>( angles[id] + angles[id + 1] ) / 2.0 )
                return id;
        }
        return angles.size() - 1;
    }

    MonsterAnimInfo GetMonsterInfo( uint32_t monsterID )
    {
        return _infoCache.getAnimInfo( monsterID );
    }
}
