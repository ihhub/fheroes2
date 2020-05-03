#ifndef H2BIN_FRM_H
#define H2BIN_FRM_H

#include "settings.h"

#include <map>
#include <vector>

class AnimationSequence;
class AnimationReference;

namespace Bin_Info
{
    struct MonsterAnimInfo
    {
        enum ANIM_TYPE
        {
            MOVE_START, // Start of the moving sequence on 1st animation cycle: flyers will fly up
            MOVE_TILE_START, // Unused? Supposed to be played at the beginning of 2nd+ move.
            MOVE_MAIN, // Core animation. Most units only have this one.
            MOVE_TILE_END, // Cavalry & wolf. Played at the end of the cycle (2nd tile to 3rd), but not at the last one
            MOVE_STOP, // End of the moving sequence when arrived: landing for example
            MOVE_ONE, // Used when moving 1 tile. LICH and POWER_LICH doesn't have this, use MOVE_MAIN
            UNUSED_WALK, // UNUSED, intended crawling speed?
            STATIC, // Frame 1
            IDLE1,
            IDLE2, // Idle animations: picked at random with different probablities, rarely all 5 present
            IDLE3,
            IDLE4,
            IDLE5,
            DEATH,
            WINCE_UP,
            WINCE_END,
            ATTACK1, // Attacks, number represents the angle: 1 is TOP, 2 is CENTER, 3 is BOTTOM
            ATTACK1_END,
            BREATH1, // Breath is 2-hex attack
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

        MonsterAnimInfo();
        MonsterAnimInfo( std::vector<u8> & data );
        bool isValid( int animID = MonsterAnimInfo::STATIC );

    private:
    };

    class MonsterAnimCache
    {
    public:
        const MonsterAnimInfo & getAnimInfo( int monsterID );
        AnimationSequence createSequence( const MonsterAnimInfo & info, int anim );
        AnimationReference createAnimReference( int monsterID );

    private:
        std::map<int, MonsterAnimInfo> _animMap;
    };

    void InitBinInfo();
    AnimationReference GetAnimationSet( int monsterID );
}

const char * GetFilename( int icnId );
#endif
