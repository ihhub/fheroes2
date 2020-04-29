#include "battle_animation.h"
#include "settings.h"
#include "bin_frm.h"

AnimationSequence::AnimationSequence( const std::map<int, std::vector<int> > & animMap, Monster::monster_t id ) : _currentSequence(_static)
{
    _type = id;

    // STATIC is our default
    // appendFrames inserts to vector so ref is still valid
    if ( !appendFrames( animMap, _static, BIN::H2_FRAME_SEQUENCE::STATIC ) ) {
        // fall back to this, to avoid crashes
        _static.push_back( 1 );
    }

    // Make sure this reference is on point !
    _currentFrame = _currentSequence.begin();

    // Taking damage
    appendFrames( animMap, _wince, BIN::H2_FRAME_SEQUENCE::WINCE_UP );
    appendFrames( animMap, _wince, BIN::H2_FRAME_SEQUENCE::WINCE_END ); // play it back together for now
    appendFrames( animMap, _death, BIN::H2_FRAME_SEQUENCE::DEATH, true );

    // Idle animations
    for ( int idx = BIN::H2_FRAME_SEQUENCE::IDLE1; idx <= BIN::H2_FRAME_SEQUENCE::IDLE5; ++idx ) {
        std::vector<int> idleAnim;
        if ( appendFrames( animMap, idleAnim, idx ) ) {
            _idle.push_back( idleAnim );
        }
    }

    // Movement sequences
    // Every unit has MOVE_MAIN anim, use it as a base
    appendFrames( animMap, _loopMove, BIN::H2_FRAME_SEQUENCE::MOVE_MAIN, true );

    if ( !animationExists( animMap, BIN::H2_FRAME_SEQUENCE::MOVE_ONE ) ) {
        // this must be LICH or POWER_LICH
        _quickMove = _loopMove;
    }
    else {
        appendFrames( animMap, _quickMove, BIN::H2_FRAME_SEQUENCE::MOVE_ONE, true );
    }

    appendFrames( animMap, _moveModes.start, BIN::H2_FRAME_SEQUENCE::MOVE_START, true );
    appendFrames( animMap, _moveModes.end, BIN::H2_FRAME_SEQUENCE::MOVE_END, true );

    // Attack sequences
    appendFrames( animMap, _melee[TOP].start, BIN::H2_FRAME_SEQUENCE::ATTACK1, true );
    appendFrames( animMap, _melee[TOP].end, BIN::H2_FRAME_SEQUENCE::ATTACK1_END );

    appendFrames( animMap, _melee[FRONT].start, BIN::H2_FRAME_SEQUENCE::ATTACK2, true );
    appendFrames( animMap, _melee[FRONT].end, BIN::H2_FRAME_SEQUENCE::ATTACK2_END );

    appendFrames( animMap, _melee[BOTTOM].start, BIN::H2_FRAME_SEQUENCE::ATTACK3, true );
    appendFrames( animMap, _melee[BOTTOM].end, BIN::H2_FRAME_SEQUENCE::ATTACK3_END );

    // Use either shooting or breath attack animation as ranged
    if ( animationExists( animMap, BIN::H2_FRAME_SEQUENCE::SHOOT2 ) ) {
        appendFrames( animMap, _ranged[TOP].start, BIN::H2_FRAME_SEQUENCE::SHOOT1, true );
        appendFrames( animMap, _ranged[TOP].end, BIN::H2_FRAME_SEQUENCE::SHOOT1_END );

        appendFrames( animMap, _ranged[FRONT].start, BIN::H2_FRAME_SEQUENCE::SHOOT2, true );
        appendFrames( animMap, _ranged[FRONT].end, BIN::H2_FRAME_SEQUENCE::SHOOT2_END );

        appendFrames( animMap, _ranged[BOTTOM].start, BIN::H2_FRAME_SEQUENCE::SHOOT3, true );
        appendFrames( animMap, _ranged[BOTTOM].end, BIN::H2_FRAME_SEQUENCE::SHOOT3_END );
    }
    else if ( animationExists( animMap, BIN::H2_FRAME_SEQUENCE::BREATH2 ) ) {
        // Only 6 units should have this
        appendFrames( animMap, _ranged[TOP].start, BIN::H2_FRAME_SEQUENCE::BREATH1, true );
        appendFrames( animMap, _ranged[TOP].end, BIN::H2_FRAME_SEQUENCE::BREATH1_END );

        appendFrames( animMap, _ranged[FRONT].start, BIN::H2_FRAME_SEQUENCE::BREATH2, true );
        appendFrames( animMap, _ranged[FRONT].end, BIN::H2_FRAME_SEQUENCE::BREATH2_END );

        appendFrames( animMap, _ranged[BOTTOM].start, BIN::H2_FRAME_SEQUENCE::BREATH3, true );
        appendFrames( animMap, _ranged[BOTTOM].end, BIN::H2_FRAME_SEQUENCE::BREATH3_END );
    }
}

AnimationSequence::~AnimationSequence() {}

bool AnimationSequence::appendFrames( const std::map<int, std::vector<int> > & animMap, std::vector<int> & target, int animID, bool critical )
{
    auto it = animMap.find( animID );
    if ( it != animMap.end() && it->second.size() > 0 ) {
        target.insert( target.end(), it->second.begin(), it->second.end() );
        return true;
    }
    // check if we're missing a very important anim
    if ( critical ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "Monster type " << _type << ", missing frames for animation: " << animID );
    }
    return false;
}

const std::vector<int> & AnimationSequence::getAnimationSequence( int animstate )
{
    switch ( animstate ) {
    case Monster::AS_STATIC:
        return _static;
        break;
    case Monster::AS_IDLE:
        return _idle.front();
        break;
    case Monster::AS_MOVE_START:
        return _moveModes.start;
        break;
    case Monster::AS_MOVING:
        return _loopMove;
        break;
    case Monster::AS_MOVE_END:
        return _moveModes.end;
        break;
    case Monster::AS_MOVE_QUICK:
        return _quickMove;
        break;
    case Monster::AS_MELEE_TOP:
        return _melee[ATTACK_DIRECTION::TOP].start;
        break;
    case Monster::AS_MELEE_TOP_END:
        return _melee[ATTACK_DIRECTION::TOP].end;
        break;
    case Monster::AS_MELEE_FRONT:
        return _melee[ATTACK_DIRECTION::FRONT].start;
        break;
    case Monster::AS_MELEE_FRONT_END:
        return _melee[ATTACK_DIRECTION::FRONT].end;
        break;
    case Monster::AS_MELEE_BOT:
        return _melee[ATTACK_DIRECTION::BOTTOM].start;
        break;
    case Monster::AS_MELEE_BOT_END:
        return _melee[ATTACK_DIRECTION::BOTTOM].end;
        break;
    case Monster::AS_RANG_TOP:
        return _ranged[ATTACK_DIRECTION::TOP].start;
        break;
    case Monster::AS_RANG_TOP_END:
        return _ranged[ATTACK_DIRECTION::TOP].end;
        break;
    case Monster::AS_RANG_FRONT:
        return _ranged[ATTACK_DIRECTION::FRONT].start;
        break;
    case Monster::AS_RANG_FRONT_END:
        return _ranged[ATTACK_DIRECTION::FRONT].end;
        break;
    case Monster::AS_RANG_BOT:
        return _ranged[ATTACK_DIRECTION::BOTTOM].start;
        break;
    case Monster::AS_RANG_BOT_END:
        return _ranged[ATTACK_DIRECTION::BOTTOM].end;
        break;
    case Monster::AS_WNCE:
        return _wince;
        break;
    case Monster::AS_KILL:
        return _death;
        break;
    case Monster::AS_MOVE:
    case Monster::AS_FLY1:
    case Monster::AS_FLY2:
    case Monster::AS_FLY3:
    case Monster::AS_SHOT0:
    case Monster::AS_SHOT1:
    case Monster::AS_SHOT2:
    case Monster::AS_SHOT3:
    case Monster::AS_ATTK0:
    case Monster::AS_ATTK1:
    case Monster::AS_ATTK2:
    case Monster::AS_ATTK3:
        DEBUG( DBG_ENGINE, DBG_WARN, "Trying to display deprecated Animation " << animstate);
    case Monster::AS_NONE:
    case Monster::AS_INVALID:
    default:
        break;
    }
    return _static;
}

int AnimationSequence::getNextFrame(const std::vector<int>& sequence, int current, bool loop)
{
    auto it = sequence.begin();
    
    // basically iterator advance operator with end checking
    // don't support negatives/going back
    while ( current > 0 && it != sequence.end() ) {
        if ( std::next(it) == sequence.end() ) {
            if ( loop ) {
                it = sequence.begin();
            }
            else {
                break;
            }            
        }
        else {
            it++;
        }
        current--;
    }
    return ( *it );
}

int AnimationSequence::switchAnimation( int animstate )
{
    _currentSequence = getAnimationSequence( animstate );    
    return restartAnimation();
}

int AnimationSequence::playAnimation( bool loop )
{
    if ( isLastFrame() ) {
        if (loop) restartAnimation();
    }
    else {
        _currentFrame++;
    }
    return ( *_currentFrame );
}

int AnimationSequence::restartAnimation()
{
    _currentFrame = _currentSequence.begin();
    return ( *_currentFrame );
}

int AnimationSequence::getFrame() const
{
    return (*_currentFrame);
}

bool AnimationSequence::isFirstFrame() const
{
    return _currentFrame == _currentSequence.begin();
}

bool AnimationSequence::isLastFrame() const
{
    return std::next(_currentFrame) == _currentSequence.end();
}

int AnimationSequence::getStaticFrame() const
{
    return _static.back();
}

int AnimationSequence::getDeadFrame() const
{
    return ( _death.empty() ) ? _static.back() : _death.back();
}