#ifndef H2BATTLE_ANIMATION_H
#define H2BATTLE_ANIMATION_H

#include "gamedefs.h"

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
    AnimationSequence( const std::vector<int> & seq );
    AnimationSequence( const AnimationSequence & );
    virtual ~AnimationSequence();

    AnimationSequence & operator=( const std::vector<int> & );
    AnimationSequence & operator=( const AnimationSequence & );

    int playAnimation( bool loop = false );
    int restartAnimation();

    int getFrame() const;
    int animationLength() const;
    double movementProgress() const;
    bool isFirstFrame() const;
    bool isLastFrame() const;

private:
    std::vector<int> _seq;
    std::vector<int>::iterator _currentFrame;
};


class AnimationReference
{
public:
    AnimationReference();
    AnimationReference( const std::map<int, std::vector<int> > & animMap, int id );
    virtual ~AnimationReference();

    int getStaticFrame() const;
    int getDeadFrame() const;

    const std::vector<int> & getAnimationVector( int animstate ) const;
    AnimationSequence getAnimationSequence( int animstate );
    static int getNextFrame( const std::vector<int> & sequence, int current = -1, bool loop = false );

protected:
    int _type;

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


class AnimationState : public AnimationReference
{
public:
    AnimationState( const std::map<int, std::vector<int> > & animMap, int id, int state );
    AnimationState( const AnimationReference & animMap, int state );
    virtual ~AnimationState();

    int switchAnimation( int animstate );
    int getCurrentState() const;

private:
    int _animState;
    AnimationSequence _currentSequence;
};
#endif