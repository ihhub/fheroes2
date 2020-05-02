#ifndef H2BIN_FRM_H
#define H2BIN_FRM_H

#include "battle_animation.h"
#include "settings.h"

#include <map>
#include <vector>

namespace Bin_Info
{

// this should be supported by VC, Clang and GCC compilers    
#pragma pack(push, 1)
    struct MonsterAnimInfo
    {
        u8 unusedFileType;           // Byte  0
        s16 blindOffset[2];          // Bytes 1 - 4
        u8 unusedMoveOffsets[7][16]; // Bytes 5 - 116
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
#pragma pack(pop)

    enum ORIGINAL_ANIMATION
    {
        MOVE_START,     // Start of the moving sequence on 1st animation cycle: flyers will fly up
        MOVE_TILE_START,// UNUSED. Supposed to be played 
        MOVE_MAIN,      // Core animation. Most units only have this one.
        MOVE_TILE_END,  // Cavalry & wolf. Played at the end of the cycle (2nd tile to 3rd), but not at the last one
        MOVE_STOP,      // End of the moving sequence when arrived: landing for example
        MOVE_ONE,       // Used when moving 1 tile. LICH and POWER_LICH doesn't have this, use MOVE_MAIN
        UNUSED_WALK,    // UNUSED, intended crawling speed?
        STATIC,         // Frame 1
        IDLE1,
        IDLE2,          // Idle animations: picked at random with different probablities, rarely all 5 present
        IDLE3,
        IDLE4,
        IDLE5,
        DEATH,
        WINCE_UP,
        WINCE_END,
        ATTACK1,        // Attacks, number represents the angle: 1 is TOP, 2 is CENTER, 3 is BOTTOM
        ATTACK1_END,
        BREATH1,        // Breath is 2-hex attack
        BREATH1_END,
        ATTACK2,
        ATTACK2_END,
        BREATH2,
        BREATH2_END,
        ATTACK3,
        ATTACK3_END,
        BREATH3,
        BREATH3_END,
        SHOOT1,
        SHOOT1_END,
        SHOOT2,
        SHOOT2_END,
        SHOOT3,
        SHOOT3_END
    };

    const char * GetFilename( int icnId );
    std::map<int, std::vector<int> > buildMonsterAnimInfo( const std::vector<u8> & data );
    bool animationExists( const std::map<int, std::vector<int> > & animMap, ORIGINAL_ANIMATION id );
    AnimationReference GetAnimationSet( int monsterID );
    bool InitBinInfo();
    void Cleanup();
    const std::map<int, std::vector<int> > & LookupBINCache( int bin_frm );
    const std::map<int, std::map<int, std::vector<int> > > & LookupBINCache();
}
#endif
