#include "bin_info.h"
#include "agg.h"
#include "battle_animation.h"
#include "monster.h"

#include <algorithm>
#include <iostream>

namespace Bin_Info
{
    const size_t CORRECT_FRM_LENGTH = 821;

    std::map<int, AnimationReference> animRefs;
    MonsterAnimCache _info_cache;

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

    const char * GetFilename( int bin_frm )
    {
        int index = Monster::UNKNOWN;

        for ( int idx = Monster::UNKNOWN; idx < Monster::WATER_ELEMENT + 1; idx++ ) {
            if ( bin_file_map[idx].type == bin_frm ) {
                index = idx;
                break;
            }
        }
        return bin_file_map[index].string;
    }

    MonsterAnimInfo::MonsterAnimInfo()
    {
        uint32_t moveSpeed;
        uint32_t shootSpeed;
        uint32_t flightSpeed;
        std::vector<std::vector<uint8_t> > frameXOffset;
        Point eyePosition;
        int32_t troopCountOffsetLeft;
        int32_t troopCountOffsetRight;
        std::vector<Point> projectileOffset;
        std::vector<float> projectileAngles;
        std::vector<float> idlePriority;
        std::vector<uint32_t> unusedIdleDelays;
        uint32_t idleAnimationDelay;
        std::vector<std::vector<int> > animationFrames;
    }

    MonsterAnimInfo::MonsterAnimInfo( std::vector<u8> & bytes ) {
        if ( bytes.size() != Bin_Info::CORRECT_FRM_LENGTH ) {
            DEBUG( DBG_ENGINE, DBG_WARN, " corrupted BIN FRM data passed in, " << bytes.size() << " length" );
        }

        // POPULATE MONSTER INFO
        uint8_t * data = bytes.data();

        eyePosition = Point( *( reinterpret_cast<int16_t *>( data + 1 ) ), *( reinterpret_cast<int16_t *>( data + 3 ) ) );

        // Frame X offsets for future use
        for ( int moveID = 0; moveID < 7; moveID++ ) {
            std::vector<uint8_t> moveOffset;
            for ( int frame = 0; frame < 16; frame++ ) {
                moveOffset.push_back( data[5 + moveID * 16 + frame] );
            }
            frameXOffset.push_back( moveOffset );
        }

        // Idle animations data
        uint8_t idleAnimationsCount = data[117];
        for ( uint8_t i = 0; i < idleAnimationsCount; i++ ) {
            idlePriority.push_back( *( reinterpret_cast<float *>( data + 118 ) + i ) );
        }
        for ( uint8_t i = 0; i < idleAnimationsCount; i++ ) {
            unusedIdleDelays[i] = *( reinterpret_cast<uint32_t *>( data + 138 ) + i );
        }
        idleAnimationDelay = *( reinterpret_cast<uint32_t *>( data + 158 ) );

        // Monster speed data
        moveSpeed = *( reinterpret_cast<uint32_t *>( data + 162 ) );
        shootSpeed = *( reinterpret_cast<uint32_t *>( data + 166 ) );
        flightSpeed = *( reinterpret_cast<uint32_t *>( data + 170 ) );

        for ( int i = 0; i < 3; i++ ) {
            projectileOffset.push_back( Point( *( reinterpret_cast<int16_t *>( data + 174 + i * 4 ) ), *( reinterpret_cast<int16_t *>( data + 176 + i * 4 ) ) ) );
        }

        uint8_t projectileCount = data[186];
        for ( uint8_t i = 0; i < projectileCount; i++ ) {
            projectileAngles.push_back( *( reinterpret_cast<float *>( data + 187 ) + i ) );
        }

        // Positional offsets for sprites & drawing
        troopCountOffsetLeft = *( reinterpret_cast<int32_t *>( data + 235 ) );
        troopCountOffsetRight = *( reinterpret_cast<int32_t *>( data + 239 ) );

        // Load animation sequences themselves
        for ( int idx = 0; idx < MonsterAnimInfo::SHOOT3_END + 1; idx++ ) {
            std::vector<int> anim;
            uint8_t count = data[243 + idx];
            for ( uint8_t frame = 0; frame < count; frame++ ) {
                anim.push_back( static_cast<int> (data[277 + idx * 16 + frame]) );
            }
            animationFrames.push_back( anim );
        }
    }

    const MonsterAnimInfo & MonsterAnimCache::getAnimInfo( int monsterID )
    {
        std::map<int, MonsterAnimInfo>::iterator mapIterator = _animMap.find( monsterID );
        if ( mapIterator == _animMap.end() ) {
            _animMap[monsterID] = MonsterAnimInfo( AGG::LoadBINFRM( Bin_Info::GetFilename( monsterID ) ) );
            if ( _animMap[monsterID].isValid() ) {
                mapIterator = _animMap.find( monsterID );
            }
            else {
                // fall back to unknown if missing data
                DEBUG( DBG_ENGINE, DBG_WARN, "missing BIN FRM data: " << Bin_Info::GetFilename( monsterID ) << ", index: " << monsterID );
                mapIterator = _animMap.find( Monster::UNKNOWN );
            }
        }
        return mapIterator->second;
    }

    bool MonsterAnimInfo::isValid( int animID )
    {
        return animationFrames.size() == MonsterAnimInfo::SHOOT3_END + 1 && animationFrames.at( animID ).size() > 0;
    }

    AnimationSequence MonsterAnimCache::createSequence( const MonsterAnimInfo & info, int animID )
    {
        return AnimationSequence( info.animationFrames.at( animID ) );
    }

    AnimationReference MonsterAnimCache::createAnimReference( int monsterID )
    {
        return AnimationReference( _info_cache.getAnimInfo( monsterID ), monsterID );
    }

    AnimationReference GetAnimationSet( int monsterID )
    {
        std::map<int, AnimationReference>::const_iterator it = animRefs.find( monsterID );
        if ( it != animRefs.end() )
            return it->second;

        return _info_cache.createAnimReference( Monster::UNKNOWN );
    }

    void InitBinInfo()
    {
        for ( int i = Monster::UNKNOWN; i < Monster::WATER_ELEMENT + 1; i++ )
            animRefs[i] = _info_cache.createAnimReference( i );
    }
}
