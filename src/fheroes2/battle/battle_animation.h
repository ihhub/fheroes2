#ifndef H2BATTLE_ANIMATION_H
#define H2BATTLE_ANIMATION_H

#include "gamedefs.h"
#include "monster_info.h"


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
    int firstFrame() const;
    int lastFrame() const;
    int animationLength() const;
    double movementProgress() const;
    bool isFirstFrame() const;
    bool isLastFrame() const;
    bool isValid() const;

private:
    std::vector<int> _seq;
    std::vector<int>::iterator _currentFrame;
    bool _reverse;

    AnimationSequence() = delete;
};

class AnimationReference
{
public:
    AnimationReference();
    AnimationReference( const std::map<int, std::vector<int> > & animMap, int id );
    AnimationReference & operator=(const AnimationReference & rhs );
    virtual ~AnimationReference();

    int getStaticFrame() const;
    int getDeadFrame() const;

    const std::vector<int> & getAnimationVector( int animstate ) const;
    AnimationSequence getAnimationSequence( int animstate ) const;
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

    bool appendFrames( const std::map<int, std::vector<int> > & animMap, std::vector<int> & target, int animID, bool critical = false );
};

class AnimationState : public AnimationReference
{
public:
    AnimationState( const std::map<int, std::vector<int> > & animMap, int id, int state );
    AnimationState( const AnimationReference & animMap, int state );
    virtual ~AnimationState();

    bool switchAnimation( int animstate, bool reverse = false );
    bool switchAnimation( const std::vector<int> & animationList, bool reverse = false );
    int getCurrentState() const;
    AnimationSequence & seq();

    // pass-down methods
    int playAnimation( bool loop = false );
    int restartAnimation( );

    int getFrame() const;
    int firstFrame() const;
    int lastFrame() const;
    int animationLength() const;
    double movementProgress() const;
    bool isFirstFrame() const;
    bool isLastFrame() const;
    bool isValid() const;

private:
    int _animState;
    AnimationSequence _currentSequence;
};
#endif
