/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "agg.h"
#include "battle_animation.h"
#include "monster.h"

#include <algorithm>
#include <iostream>

namespace Bin_Info
{
    class MonsterAnimCache
    {
    public:
        AnimationSequence createSequence( const MonsterAnimInfo & info, int anim );
        AnimationReference createAnimReference( int monsterID );

    private:
        std::map<int, MonsterAnimInfo> _animMap;

        MonsterAnimInfo getAnimInfo( int monsterID );
    };

    const size_t CORRECT_FRM_LENGTH = 821;

    std::map<int, AnimationReference> animRefs;
    MonsterAnimCache _infoCache;

    const struct
    {
        int type;
        const char * string;
    } bin_file_map[] = {{Monster::UNKNOWN, "UNKNOWN"},
                        {Monster::ARCHER, "ARCHRFRM.BIN"},
                        {Monster::RANGER, "ARCHRFRM.BIN"},
                        {Monster::BOAR, "BOAR_FRM.BIN"},
                        {Monster::CENTAUR, "CENTRFRM.BIN"},
                        {Monster::CHAMPION, "CVLR2FRM.BIN"},
                        {Monster::CAVALRY, "CVLRYFRM.BIN"},
                        {Monster::CYCLOPS, "CYCLOFRM.BIN"},
                        {Monster::BONE_DRAGON, "DRABNFRM.BIN"},
                        {Monster::BLACK_DRAGON, "DRAGBFRM.BIN"},
                        {Monster::GREEN_DRAGON, "DRAGGFRM.BIN"},
                        {Monster::RED_DRAGON, "DRAGRFRM.BIN"},
                        {Monster::DRUID, "DRUIDFRM.BIN"},
                        {Monster::GREATER_DRUID, "DRUIDFRM.BIN"},
                        {Monster::DWARF, "DWARFFRM.BIN"},
                        {Monster::BATTLE_DWARF, "DWARFFRM.BIN"},
                        {Monster::ELF, "ELF__FRM.BIN"},
                        {Monster::GRAND_ELF, "ELF__FRM.BIN"},
                        {Monster::FIRE_ELEMENT, "FELEMFRM.BIN"},
                        {Monster::EARTH_ELEMENT, "FELEMFRM.BIN"},
                        {Monster::AIR_ELEMENT, "FELEMFRM.BIN"},
                        {Monster::WATER_ELEMENT, "FELEMFRM.BIN"},
                        {Monster::GARGOYLE, "GARGLFRM.BIN"},
                        {Monster::GENIE, "GENIEFRM.BIN"},
                        {Monster::GHOST, "GHOSTFRM.BIN"},
                        {Monster::GOBLIN, "GOBLNFRM.BIN"},
                        {Monster::IRON_GOLEM, "GOLEMFRM.BIN"},
                        {Monster::STEEL_GOLEM, "GOLEMFRM.BIN"},
                        {Monster::GRIFFIN, "GRIFFFRM.BIN"},
                        {Monster::HALFLING, "HALFLFRM.BIN"},
                        {Monster::HYDRA, "HYDRAFRM.BIN"},
                        {Monster::LICH, "LICH_FRM.BIN"},
                        {Monster::POWER_LICH, "LICH_FRM.BIN"},
                        {Monster::MAGE, "MAGE1FRM.BIN"},
                        {Monster::ARCHMAGE, "MAGE1FRM.BIN"},
                        {Monster::MEDUSA, "MEDUSFRM.BIN"},
                        {Monster::MINOTAUR, "MINOTFRM.BIN"},
                        {Monster::MINOTAUR_KING, "MINOTFRM.BIN"},
                        {Monster::MUMMY, "MUMMYFRM.BIN"},
                        {Monster::ROYAL_MUMMY, "MUMMYFRM.BIN"},
                        {Monster::NOMAD, "NOMADFRM.BIN"},
                        {Monster::OGRE, "OGRE_FRM.BIN"},
                        {Monster::OGRE_LORD, "OGRE_FRM.BIN"},
                        {Monster::ORC, "ORC__FRM.BIN"},
                        {Monster::ORC_CHIEF, "ORC__FRM.BIN"},
                        {Monster::PALADIN, "PALADFRM.BIN"},
                        {Monster::CRUSADER, "PALADFRM.BIN"},
                        {Monster::PEASANT, "PEAS_FRM.BIN"},
                        {Monster::PHOENIX, "PHOENFRM.BIN"},
                        {Monster::PIKEMAN, "PIKMNFRM.BIN"},
                        {Monster::VETERAN_PIKEMAN, "PIKMNFRM.BIN"},
                        {Monster::ROC, "ROC__FRM.BIN"},
                        {Monster::ROGUE, "ROGUEFRM.BIN"},
                        {Monster::SKELETON, "SKEL_FRM.BIN"},
                        {Monster::SPRITE, "SPRITFRM.BIN"},
                        {Monster::SWORDSMAN, "SWRDSFRM.BIN"},
                        {Monster::MASTER_SWORDSMAN, "SWRDSFRM.BIN"},
                        {Monster::TITAN, "TITA2FRM.BIN"},
                        {Monster::GIANT, "TITANFRM.BIN"},
                        {Monster::TROLL, "TROLLFRM.BIN"},
                        {Monster::WAR_TROLL, "TROLLFRM.BIN"},
                        {Monster::UNICORN, "UNICOFRM.BIN"},
                        {Monster::VAMPIRE, "VAMPIFRM.BIN"},
                        {Monster::VAMPIRE_LORD, "VAMPIFRM.BIN"},
                        {Monster::WOLF, "WOLF_FRM.BIN"},
                        {Monster::ZOMBIE, "ZOMB_FRM.BIN"},
                        {Monster::MUTANT_ZOMBIE, "ZOMB_FRM.BIN"}};

    const char * GetFilename( int monsterId )
    {
        int index = Monster::UNKNOWN;

        for ( int idx = Monster::UNKNOWN; idx < Monster::WATER_ELEMENT + 1; ++idx ) {
            if ( bin_file_map[idx].type == monsterId ) {
                index = idx;
                break;
            }
        }
        return bin_file_map[index].string;
    }

    MonsterAnimInfo::MonsterAnimInfo( const std::vector<u8> & bytes )
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

        eyePosition = Point( *( reinterpret_cast<const int16_t *>( data + 1 ) ), *( reinterpret_cast<const int16_t *>( data + 3 ) ) );

        // Frame X offsets for the future use
        for ( int moveID = 0; moveID < 7; ++moveID ) {
            std::vector<uint8_t> moveOffset;
            for ( int frame = 0; frame < 16; ++frame ) {
                moveOffset.push_back( data[5 + moveID * 16 + frame] );
            }
            frameXOffset.push_back( moveOffset );
        }

        // Idle animations data
        idleAnimationCount = data[117];
        if ( idleAnimationCount > 5u )
            idleAnimationCount = 5u; // here we need to reset our object
        for ( uint8_t i = 0; i < idleAnimationCount; ++i )
            idlePriority.push_back( *( reinterpret_cast<const float *>( data + 118 ) + i ) );

        for ( uint8_t i = 0; i < idleAnimationCount; ++i )
            unusedIdleDelays.push_back( *( reinterpret_cast<const uint32_t *>( data + 138 ) + i ) );

        idleAnimationDelay = *( reinterpret_cast<const uint32_t *>( data + 158 ) );

        // Monster speed data
        moveSpeed = *( reinterpret_cast<const uint32_t *>( data + 162 ) );
        shootSpeed = *( reinterpret_cast<const uint32_t *>( data + 166 ) );
        flightSpeed = *( reinterpret_cast<const uint32_t *>( data + 170 ) );

        // Projectile data
        for ( size_t i = 0; i < 3; ++i ) {
            projectileOffset.push_back(
                Point( *( reinterpret_cast<const int16_t *>( data + 174 + ( i * 4 ) ) ), *( reinterpret_cast<const int16_t *>( data + 176 + ( i * 4 ) ) ) ) );
        }

        uint8_t projectileCount = data[186];
        if ( projectileCount > 12u )
            projectileCount = 12u; // here we need to reset our object
        for ( uint8_t i = 0; i < projectileCount; ++i )
            projectileAngles.push_back( *( reinterpret_cast<const float *>( data + 187 ) + i ) );

        // Positional offsets for sprites & drawing
        troopCountOffsetLeft = *( reinterpret_cast<const int32_t *>( data + 235 ) );
        troopCountOffsetRight = *( reinterpret_cast<const int32_t *>( data + 239 ) );

        // Load animation sequences themselves
        for ( int idx = MOVE_START; idx <= SHOOT3_END; ++idx ) {
            std::vector<int> anim;
            uint8_t count = data[243 + idx];
            if ( count > 16 )
                count = 16; // here we need to reset our object
            for ( uint8_t frame = 0; frame < count; frame++ ) {
                anim.push_back( static_cast<int>( data[277 + idx * 16 + frame] ) );
            }
            animationFrames.push_back( anim );
        }
    }

    MonsterAnimInfo MonsterAnimCache::getAnimInfo( int monsterID )
    {
        std::map<int, MonsterAnimInfo>::iterator mapIterator = _animMap.find( monsterID );
        if ( mapIterator != _animMap.end() ) {
            return mapIterator->second;
        }
        else {
            const MonsterAnimInfo info( AGG::LoadBINFRM( Bin_Info::GetFilename( monsterID ) ) );
            if ( info.isValid() ) {
                _animMap[monsterID] = info;
                return info;
            }
            else {
                DEBUG( DBG_ENGINE, DBG_WARN, "missing BIN FRM data: " << Bin_Info::GetFilename( monsterID ) << ", index: " << monsterID );
            }
        }
        return MonsterAnimInfo();
    }

    bool MonsterAnimInfo::isValid() const
    {
        if ( animationFrames.size() != SHOOT3_END + 1 )
            return false;

        // Absolute minimal set up
        const int essentialAnimations[7] = {MOVE_MAIN, STATIC, DEATH, WINCE_UP, ATTACK1, ATTACK2, ATTACK3};

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

    AnimationSequence MonsterAnimCache::createSequence( const MonsterAnimInfo & info, int animID )
    {
        return AnimationSequence( info.animationFrames.at( animID ) );
    }

    AnimationReference MonsterAnimCache::createAnimReference( int monsterID )
    {
        return AnimationReference( _infoCache.getAnimInfo( monsterID ), monsterID );
    }

    AnimationReference GetAnimationSet( int monsterID )
    {
        std::map<int, AnimationReference>::const_iterator it = animRefs.find( monsterID );
        if ( it != animRefs.end() )
            return it->second;

        return _infoCache.createAnimReference( Monster::UNKNOWN );
    }

    void InitBinInfo()
    {
        for ( int i = Monster::UNKNOWN; i < Monster::WATER_ELEMENT + 1; ++i )
            animRefs[i] = _infoCache.createAnimReference( i );
    }
}
