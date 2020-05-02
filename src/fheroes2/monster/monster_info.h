#ifndef H2MONSTER_INFO_H
#define H2MONSTER_INFO_H

namespace Monster_State
{
    enum ATTACK_DIRECTION
    {
        TOP,
        FRONT,
        BOTTOM
    };

    enum ANIMATION_TYPE
    {
        NONE,
        STATIC,
        IDLE,

        IDLE_ANY,
        MOVE_START,
        MOVING,
        MOVE_END,
        MOVE_QUICK,
        MELEE_TOP,
        MELEE_TOP_END,
        MELEE_FRONT,
        MELEE_FRONT_END,
        MELEE_BOT,
        MELEE_BOT_END,
        RANG_TOP,
        RANG_TOP_END,
        RANG_FRONT,
        RANG_FRONT_END,
        RANG_BOT,
        RANG_BOT_END,
        WNCE, // combine UP+DOWN anim for now
        KILL,

        // Old states to remove after refactor:
        MOVE,
        FLY1,
        FLY2,
        FLY3,
        SHOT0,
        SHOT1,
        SHOT2,
        SHOT3,
        ATTK0,
        ATTK1,
        ATTK2,
        ATTK3,

        INVALID
    };
}
#endif