#ifndef H2BATTLE_ANIMATION_H
#define H2BATTLE_ANIMATION_H

#include "gamedefs.h"
#include "monster.h"

enum ATTACK_DIRECTION
{
    TOP,
    FRONT,
    BOTTOM,
    DIRECTION_END
};

struct startEndAnim_t
{
    std::vector<int> start;
    std::vector<int> end;
};

class AnimationSequence
{
public:
    AnimationSequence( const std::map<int, std::vector<int> > & animMap, Monster::monster_t id );
    ~AnimationSequence();


    int switchAnimation( int animstate );
    int playAnimation( bool loop = false );
    int restartAnimation();

    int getFrame() const;
    bool isFirstFrame() const;
    bool isLastFrame() const;
    int getStaticFrame() const;
    int getDeadFrame() const;
    
    const std::vector<int> & getAnimationSequence( int animstate );
    static int getNextFrame( const std::vector<int> & sequence, int current = -1, bool loop = false );

private:
    Monster::monster_t _type;

    // These two members can potentially be refactored into true AnimationSequence class
    // Then the parent will become AnimationSet holder
    std::vector<int> & _currentSequence;
    std::vector<int>::iterator _currentFrame;

    std::vector<int> _static;
    std::vector<int> _quickMove;
    std::vector<int> _loopMove;
    startEndAnim_t _moveModes;
    std::vector<int> _wince;
    std::vector<int> _death;
    startEndAnim_t _melee[ATTACK_DIRECTION::DIRECTION_END];
    startEndAnim_t _ranged[ATTACK_DIRECTION::DIRECTION_END];
    std::vector<std::vector<int> > _idle;

    int _frameDelay[4]; // TODO: extract and find if it's useful later

    bool appendFrames( const std::map<int, std::vector<int> > & animMap, std::vector<int> & target, int animID, bool critical = false );
};

#endif