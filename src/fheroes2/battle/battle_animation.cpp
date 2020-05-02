#include "battle_animation.h"
#include "bin_frm.h"
#include "monster.h"
#include "settings.h"

AnimationSequence::AnimationSequence( const std::vector<int> & seq )
    : _seq( seq )
{
    // Do we need this?
    if ( _seq.empty() ) {
        _seq.push_back( 1 );
    }

    // Make sure this reference is on point !
    _currentFrame =  _seq.begin();
}

AnimationSequence::AnimationSequence( const AnimationSequence & rhs )
    : _seq( rhs._seq )
{
    if (_seq.empty()) DEBUG( DBG_GAME, DBG_WARN, " AnimationSequence is empty C1! " << _seq.size() );
    _currentFrame = _seq.begin();
}

AnimationSequence & AnimationSequence::operator=( const std::vector<int> & rhs )
{
    _seq = rhs;
    _currentFrame = _seq.begin();
    return *this;
}

AnimationSequence & AnimationSequence::operator=( const AnimationSequence & rhs )
{
    _seq = rhs._seq;
    _currentFrame = _seq.begin();
    return *this;
}

AnimationSequence::~AnimationSequence() {}

int AnimationSequence::playAnimation( bool loop )
{
    if ( isLastFrame() ) {
        if ( loop )
            restartAnimation();
    }
    else {
        _currentFrame++;
    }
    return *_currentFrame;
}

int AnimationSequence::restartAnimation()
{
    _currentFrame = _seq.begin();
    return getFrame();
}

int AnimationSequence::getFrame() const
{
    return isValid() ? *_currentFrame : lastFrame();
}

int AnimationSequence::animationLength() const
{
    return _seq.size();
}

int AnimationSequence::firstFrame() const
{
    return isValid() ? _seq.front() : 0;
}

int AnimationSequence::lastFrame() const
{
    return isValid() ? _seq.back() : 0;
}

double AnimationSequence::movementProgress() const
{
    if ( isValid() )
        return static_cast<double>( _currentFrame - _seq.begin() ) / animationLength();

    return 1.0;
}

bool AnimationSequence::isFirstFrame() const
{
    return isValid() ? _currentFrame == _seq.begin() : false;
}

bool AnimationSequence::isLastFrame() const
{
    return isValid() ? std::next( _currentFrame ) == _seq.end() : false;
}

bool AnimationSequence::isValid() const
{
    if ( _seq.size() == 0 ) {
        return false;
    }
    else if (_currentFrame == _seq.end()) {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationSequence has " << _seq.size() << " frames but currentFrame is in invalid state" );
        return false;
    }
    return true;
}

AnimationState::AnimationState( const std::map<int, std::vector<int> > & animMap, int id, int state )
    : AnimationReference( animMap, id )
    , _currentSequence(_static)
{
    switchAnimation( state );
}


AnimationState::AnimationState( const AnimationReference & ref, int state )
    : AnimationReference( ref )
    , _currentSequence(_static)
{
    switchAnimation( state );
}

AnimationState::~AnimationState() {}

bool AnimationState::switchAnimation( int animstate, bool reverse )
{
    auto seq = getAnimationVector( animstate );
    if ( seq.size() > 0 ) {
        _animState = animstate;
        if ( reverse )
            std::reverse( seq.begin(), seq.end() );
        _currentSequence = seq;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim " << animstate << " length " << _currentSequence.animationLength() );
    }
    return false;
}

bool AnimationState::switchAnimation( const std::vector<int> & animationList, bool reverse )
{
    std::vector<int> combinedAnimation;

    for ( std::vector<int>::const_iterator it = animationList.begin(); it != animationList.end(); ++it ) {
        auto seq = getAnimationVector( *it );
        if ( seq.size() > 0 ) {
            _animState = *it;
            combinedAnimation.insert( combinedAnimation.end(), seq.begin(), seq.end() );
        }
    }

    if ( combinedAnimation.size() > 0 ) {
        if ( reverse )
            std::reverse( combinedAnimation.begin(), combinedAnimation.end() );

        _currentSequence = combinedAnimation;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim list of length " << animationList.size() );
    }
    return false;
}

int AnimationState::getCurrentState( ) const
{
    return _animState;
}

AnimationSequence & AnimationState::seq()
{
    return _currentSequence;
}

int AnimationState::playAnimation( bool loop )
{
    return _currentSequence.playAnimation( loop );
}

int AnimationState::restartAnimation()
{
    return _currentSequence.restartAnimation();
}

int AnimationState::getFrame() const
{
    return _currentSequence.getFrame();
}

int AnimationState::animationLength() const
{
    return _currentSequence.animationLength();
}

int AnimationState::firstFrame() const
{
    return _currentSequence.firstFrame();
}

int AnimationState::lastFrame() const
{
    return _currentSequence.lastFrame();
}

double AnimationState::movementProgress() const
{
    return _currentSequence.movementProgress();
}

bool AnimationState::isFirstFrame() const
{
    return _currentSequence.isFirstFrame();
}

bool AnimationState::isLastFrame() const
{
    return _currentSequence.isLastFrame();
}

bool AnimationState::isValid() const
{
    return _currentSequence.isValid();
}

AnimationReference::AnimationReference()
{
    _type = Monster::UNKNOWN;
}

AnimationReference::AnimationReference( const std::map<int, std::vector<int> > & animMap, int id )
{
    _type = id;

    // STATIC is our default
    // appendFrames inserts to vector so ref is still valid
    if ( !appendFrames( animMap, _static, BIN::H2_FRAME_SEQUENCE::STATIC ) ) {
        // fall back to this, to avoid crashes
        _static.push_back( 1 );
    }

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

    appendFrames( animMap, _moveModes.start, BIN::H2_FRAME_SEQUENCE::MOVE_START );
    appendFrames( animMap, _moveModes.end, BIN::H2_FRAME_SEQUENCE::MOVE_END );

    // Attack sequences
    appendFrames( animMap, _melee[ATTACK_DIRECTION::TOP].start, BIN::H2_FRAME_SEQUENCE::ATTACK1, true );
    appendFrames( animMap, _melee[ATTACK_DIRECTION::TOP].end, BIN::H2_FRAME_SEQUENCE::ATTACK1_END );

    appendFrames( animMap, _melee[ATTACK_DIRECTION::FRONT].start, BIN::H2_FRAME_SEQUENCE::ATTACK2, true );
    appendFrames( animMap, _melee[ATTACK_DIRECTION::FRONT].end, BIN::H2_FRAME_SEQUENCE::ATTACK2_END );

    appendFrames( animMap, _melee[ATTACK_DIRECTION::BOTTOM].start, BIN::H2_FRAME_SEQUENCE::ATTACK3, true );
    appendFrames( animMap, _melee[ATTACK_DIRECTION::BOTTOM].end, BIN::H2_FRAME_SEQUENCE::ATTACK3_END );

    // Use either shooting or breath attack animation as ranged
    if ( animationExists( animMap, BIN::H2_FRAME_SEQUENCE::SHOOT2 ) ) {
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].start, BIN::H2_FRAME_SEQUENCE::SHOOT1, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].end, BIN::H2_FRAME_SEQUENCE::SHOOT1_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].start, BIN::H2_FRAME_SEQUENCE::SHOOT2, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].end, BIN::H2_FRAME_SEQUENCE::SHOOT2_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].start, BIN::H2_FRAME_SEQUENCE::SHOOT3, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].end, BIN::H2_FRAME_SEQUENCE::SHOOT3_END );
    }
    else if ( animationExists( animMap, BIN::H2_FRAME_SEQUENCE::BREATH2 ) ) {
        // Only 6 units should have this
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].start, BIN::H2_FRAME_SEQUENCE::BREATH1, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].end, BIN::H2_FRAME_SEQUENCE::BREATH1_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].start, BIN::H2_FRAME_SEQUENCE::BREATH2, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].end, BIN::H2_FRAME_SEQUENCE::BREATH2_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].start, BIN::H2_FRAME_SEQUENCE::BREATH3, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].end, BIN::H2_FRAME_SEQUENCE::BREATH3_END );
    }
}

AnimationReference & AnimationReference::operator=( const AnimationReference & rhs ) 
{
    //int _type;

    //std::vector<int> _static;
    //std::vector<int> _quickMove;
    //std::vector<int> _loopMove;
    //startEndAnim_t _moveModes;
    //std::vector<int> _wince;
    //std::vector<int> _death;
    //startEndAnim_t _melee[ATTACK_DIRECTION::DIRECTION_END];
    //startEndAnim_t _ranged[ATTACK_DIRECTION::DIRECTION_END];
    //std::vector<std::vector<int> > _idle;

    _type = rhs._type;
    _static = rhs._static;
    _loopMove = rhs._loopMove;
    _wince = rhs._wince;
    _death = rhs._death;
    _idle = rhs._idle;

    //_moveModes.start = rhs._moveModes.start;
    //_moveModes.end = rhs._moveModes.end;

    for ( int i = 0; i < 3; i++ ) {
        //_melee[i].start = rhs._melee[i].start;
        //_melee[i].end = rhs._melee[i].end;
        //_ranged[i].start = rhs._ranged[i].start;
        //_ranged[i].end = rhs._ranged[i].end;
    }
    return *this;
}

AnimationReference::~AnimationReference() {}

bool AnimationReference::appendFrames( const std::map<int, std::vector<int> > & animMap, std::vector<int> & target, int animID, bool critical )
{
    std::map<int, std::vector<int> >::const_iterator it = animMap.find( animID );
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

const std::vector<int> & AnimationReference::getAnimationVector( int animstate ) const
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
    case Monster::AS_MOVE:
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
    default:
        DEBUG( DBG_ENGINE, DBG_WARN, "Trying to display deprecated Animation " << animstate );
        break;
    }
    return _static;
}

AnimationSequence AnimationReference::getAnimationSequence( int animstate ) const
{
    return AnimationSequence( getAnimationVector( animstate ) );
}

int AnimationReference::getNextFrame( const std::vector<int> & sequence, int current, bool loop )
{
    std::vector<int>::const_iterator it = sequence.begin();

    // basically iterator advance operator with end checking
    // don't support negatives/going back
    while ( current > 0 && it != sequence.end() ) {
        if ( std::next( it ) == sequence.end() ) {
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

int AnimationReference::getStaticFrame() const
{
    return _static.back();
}

int AnimationReference::getDeadFrame() const
{
    return ( _death.empty() ) ? _static.back() : _death.back();
}
