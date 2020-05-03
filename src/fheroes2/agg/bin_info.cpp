#include "agg.h"
#include "battle_animation.h"
#include "bin_info.h"
#include "monster.h"

#include <algorithm>
#include <iostream>

namespace Bin_Info
{
    const size_t CORRECT_FRM_LENGTH = 821;

// this pragma should be supported by VC, Clang and GCC compilers
#pragma pack( push, 1 )
    // Byte-wise mapping of original BIN FRM tile for file reading purposes
    struct FRMFileStructure
    {
        u8 unusedFileType;           // Byte  0
        s16 eyePosition[2];          // Bytes 1 - 4
        u8 frameXOffset[7][16];      // Bytes 5 - 116
        u8 idleAnimationsCount;      // Byte  117
        float idlePriority[5];       // Bytes 118 - 137
        u32 unusedIdleDelays[5];     // Bytes 138 - 157
        u32 idleAnimationDelay;      // Bytes 158 - 161
        u32 moveSpeed;               // Bytes 162 - 165
        u32 shootSpeed;              // Bytes 166 - 169
        u32 flightSpeed;             // Bytes 170 - 173
        s16 projectileOffset[3][2];  // Bytes 174 - 185
        u8 projectileCount;          // Byte  186
        float projectileAngles[12];  // Bytes 187 - 234
        s32 troopCountOffsetLeft;    // Bytes 235 - 238
        s32 troopCountOffsetRight;   // Bytes 239 - 242
        u8 animationLength[34];      // Bytes 243 - 276
        u8 animationFrames[34][16];  // Bytes 277 - 820 
    };
#pragma pack( pop )

    std::map<int, AnimationReference> animRefs;
    MonsterAnimCache _info_cache;

    const struct
    {
        int type;
        const char * string;
    } bin_file_map[] = {
        {Monster::UNKNOWN, "UNKNOWN"},         {Monster::ARCHER, "ARCHRFRM.BIN"},       {Monster::RANGER, "ARCHRFRM.BIN"},      {Monster::BOAR, "BOAR_FRM.BIN"},        
        {Monster::CENTAUR, "CENTRFRM.BIN"},    {Monster::CHAMPION, "CVLR2FRM.BIN"},     {Monster::CAVALRY, "CVLRYFRM.BIN"},     {Monster::CYCLOPS, "CYCLOFRM.BIN"},
        {Monster::BONE_DRAGON, "DRABNFRM.BIN"},{Monster::BLACK_DRAGON, "DRAGBFRM.BIN"}, {Monster::GREEN_DRAGON, "DRAGGFRM.BIN"},{Monster::RED_DRAGON, "DRAGRFRM.BIN"}, 
        {Monster::DRUID, "DRUIDFRM.BIN"},      {Monster::GREATER_DRUID, "DRUIDFRM.BIN"},{Monster::DWARF, "DWARFFRM.BIN"},       {Monster::BATTLE_DWARF, "DWARFFRM.BIN"},
        {Monster::ELF, "ELF__FRM.BIN"},        {Monster::GRAND_ELF, "ELF__FRM.BIN"},    {Monster::FIRE_ELEMENT, "FELEMFRM.BIN"},{Monster::EARTH_ELEMENT, "FELEMFRM.BIN"}, 
        {Monster::AIR_ELEMENT, "FELEMFRM.BIN"},{Monster::WATER_ELEMENT, "FELEMFRM.BIN"},{Monster::GARGOYLE, "GARGLFRM.BIN"},    {Monster::GENIE, "GENIEFRM.BIN"},    
        {Monster::GHOST, "GHOSTFRM.BIN"},      {Monster::GOBLIN, "GOBLNFRM.BIN"},       {Monster::IRON_GOLEM, "GOLEMFRM.BIN"},  {Monster::STEEL_GOLEM, "GOLEMFRM.BIN"},
        {Monster::GRIFFIN, "GRIFFFRM.BIN"},    {Monster::HALFLING, "HALFLFRM.BIN"},     {Monster::HYDRA, "HYDRAFRM.BIN"},       {Monster::LICH, "LICH_FRM.BIN"},        
        {Monster::POWER_LICH, "LICH_FRM.BIN"}, {Monster::MAGE, "MAGE1FRM.BIN"},         {Monster::ARCHMAGE, "MAGE1FRM.BIN"},    {Monster::MEDUSA, "MEDUSFRM.BIN"},    
        {Monster::MINOTAUR, "MINOTFRM.BIN"},   {Monster::MINOTAUR_KING, "MINOTFRM.BIN"},{Monster::MUMMY, "MUMMYFRM.BIN"},       {Monster::ROYAL_MUMMY, "MUMMYFRM.BIN"},
        {Monster::NOMAD, "NOMADFRM.BIN"},      {Monster::OGRE, "OGRE_FRM.BIN"},         {Monster::OGRE_LORD, "OGRE_FRM.BIN"},   {Monster::ORC, "ORC__FRM.BIN"},     
        {Monster::ORC_CHIEF, "ORC__FRM.BIN"},  {Monster::PALADIN, "PALADFRM.BIN"},      {Monster::CRUSADER, "PALADFRM.BIN"},    {Monster::PEASANT, "PEAS_FRM.BIN"}, 
        {Monster::PHOENIX, "PHOENFRM.BIN"},    {Monster::PIKEMAN, "PIKMNFRM.BIN"},      {Monster::VETERAN_PIKEMAN, "PIKMNFRM.BIN"},{Monster::ROC, "ROC__FRM.BIN"},    
        {Monster::ROGUE, "ROGUEFRM.BIN"},      {Monster::SKELETON, "SKEL_FRM.BIN"},     {Monster::SPRITE, "SPRITFRM.BIN"},      {Monster::SWORDSMAN, "SWRDSFRM.BIN"}, 
        {Monster::MASTER_SWORDSMAN, "SWRDSFRM.BIN"},{Monster::TITAN, "TITA2FRM.BIN"},   {Monster::GIANT, "TITANFRM.BIN"},       {Monster::TROLL, "TROLLFRM.BIN"},       
        {Monster::WAR_TROLL, "TROLLFRM.BIN"},  {Monster::UNICORN, "UNICOFRM.BIN"},      {Monster::VAMPIRE, "VAMPIFRM.BIN"},     {Monster::VAMPIRE_LORD, "VAMPIFRM.BIN"}, 
        {Monster::WOLF, "WOLF_FRM.BIN"},       {Monster::ZOMBIE, "ZOMB_FRM.BIN"},       {Monster::MUTANT_ZOMBIE, "ZOMB_FRM.BIN"}
    };

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

    MonsterAnimInfo MonsterAnimCache::buildMonsterAnimInfo( const std::vector<u8> & bytes )
    {
        FRMFileStructure dataSet;
        MonsterAnimInfo monster_info;

        std::map<int, std::vector<int> > animationMap;

        // sanity check
        if ( sizeof( FRMFileStructure ) != Bin_Info::CORRECT_FRM_LENGTH ) {
            DEBUG( DBG_ENGINE, DBG_WARN, "Size of MonsterAnimInfo does not match expected length: " << sizeof( MonsterAnimInfo ) );
            return monster_info;
        }

        if ( bytes.size() != sizeof( FRMFileStructure ) ) {
            DEBUG( DBG_ENGINE, DBG_WARN, " wrong/corrupted BIN FRM FILE: " << bytes.size() << " length" );
            return monster_info;
        }

        // populate struct via direct copy, when size aligns
        std::memcpy( &dataSet, bytes.data(), sizeof( FRMFileStructure ) );

        // POPULATE MONSTER INFO
        // Monster speed data
        monster_info.moveSpeed = dataSet.moveSpeed;
        monster_info.shootSpeed = dataSet.shootSpeed;
        monster_info.flightSpeed = dataSet.flightSpeed;

        // Positional offsets for sprites & drawing
        monster_info.eyePosition = Point( dataSet.eyePosition[0], dataSet.eyePosition[1] );
        monster_info.troopCountOffsetLeft = dataSet.troopCountOffsetLeft;
        monster_info.troopCountOffsetRight = dataSet.troopCountOffsetRight;

        for ( int i = 0; i < 3; i++ ) {
            monster_info.projectileOffset[i] = Point( dataSet.projectileOffset[i][0], dataSet.projectileOffset[i][1] );
        }
        for ( int i = 0; i < dataSet.projectileCount; i++ ) {
            monster_info.projectileAngles.push_back( dataSet.projectileAngles[i] );
        }

        // Frame X offsets for future use
        for ( int moveID = 0; moveID < 7; moveID++ ) {
            for ( int frame = 0; frame < 16; frame++ ) {
                monster_info.frameXOffset[moveID][frame] = dataSet.frameXOffset[moveID][frame];
            }
        }

        // Idle animations data
        monster_info.idleAnimationsCount = dataSet.idleAnimationsCount;
        monster_info.idleDelay = dataSet.idleAnimationDelay;
        for ( int i = 0; i < dataSet.idleAnimationsCount; i++ ) {
            monster_info.idlePriority[i] = dataSet.idlePriority[i];
        }

        // Load animation sequences themselves
        for ( int idx = 0; idx < MonsterAnimInfo::SHOOT3_END + 1; idx++ ) {
            std::vector<int> anim;
            for ( int frame = 0; frame < dataSet.animationLength[idx]; frame++ ) {
                anim.push_back( dataSet.animationFrames[idx][frame] );
            }
            monster_info.animations.push_back( anim );
        }

        return monster_info;
    }

    MonsterAnimCache::MonsterAnimCache()
    {
        _animMap[Monster::UNKNOWN] = MonsterAnimInfo();
    }

    bool MonsterAnimCache::populate( int monsterID )
    {
        std::vector<u8> body = AGG::LoadBINFRM( Bin_Info::GetFilename( monsterID ) );

        if ( body.size() ) {
            MonsterAnimInfo monsterInfo = buildMonsterAnimInfo( body );
            if ( isMonsterInfoValid( monsterInfo ) ) {
                _animMap[monsterID] = monsterInfo;
                return true;
            }
        }
        return false;
    }

    const MonsterAnimInfo & MonsterAnimCache::getAnimInfo( int monsterID )
    {
        std::map<int, MonsterAnimInfo>::iterator mapIterator = _animMap.find( monsterID );
        if ( mapIterator == _animMap.end() ) {
            if ( populate( monsterID ) ) {
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

    bool MonsterAnimCache::isMonsterInfoValid( const MonsterAnimInfo & info, int animID )
    {
        return info.animations.size() == MonsterAnimInfo::SHOOT3_END + 1 && info.animations.at( animID ).size() > 0;
    }

    AnimationSequence MonsterAnimCache::createSequence( const MonsterAnimInfo & info, int animID )
    {
        return AnimationSequence( info.animations.at( animID ) );
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
