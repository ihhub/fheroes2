#include "battle_animation.h"
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

AnimationReference::AnimationReference( const Bin_Info::MonsterAnimInfo & info, int id )
{
    _type = id;

    // STATIC is our default
    // appendFrames inserts to vector so ref is still valid
    if ( !appendFrames( info, _static, Bin_Info::STATIC ) )
    {
        // fall back to this, to avoid crashes
        _static.push_back( 1 );
    }

    // Taking damage
    appendFrames( info, _wince, Bin_Info::WINCE_UP );
    appendFrames( info, _wince, Bin_Info::WINCE_END ); // play it back together for now
    appendFrames( info, _death, Bin_Info::DEATH, true );

    // Idle animations
    for ( int idx = Bin_Info::IDLE1; idx < info.idleAnimationsCount + Bin_Info::IDLE1; ++idx ) {
        std::vector<int> idleAnim;

        if ( appendFrames( info, idleAnim, idx ) ) {
            _idle.push_back( idleAnim );
        }
    }

    // Movement sequences
    // Every unit has MOVE_MAIN anim, use it as a base
    appendFrames( info, _loopMove, Bin_Info::MOVE_MAIN, true );

    if ( info.animationLength[Bin_Info::MOVE_ONE] > 0 ) {
        appendFrames( info, _quickMove, Bin_Info::MOVE_ONE, true );
    }
    else {
        // this must be LICH or POWER_LICH
        _quickMove = _loopMove;
    }

    appendFrames( info, _moveModes.start, Bin_Info::MOVE_START );
    appendFrames( info, _moveModes.end, Bin_Info::MOVE_STOP );

    // Attack sequences
    appendFrames( info, _melee[ATTACK_DIRECTION::TOP].start, Bin_Info::ATTACK1, true );
    appendFrames( info, _melee[ATTACK_DIRECTION::TOP].end, Bin_Info::ATTACK1_END );

    appendFrames( info, _melee[ATTACK_DIRECTION::FRONT].start, Bin_Info::ATTACK2, true );
    appendFrames( info, _melee[ATTACK_DIRECTION::FRONT].end, Bin_Info::ATTACK2_END );

    appendFrames( info, _melee[ATTACK_DIRECTION::BOTTOM].start, Bin_Info::ATTACK3, true );
    appendFrames( info, _melee[ATTACK_DIRECTION::BOTTOM].end, Bin_Info::ATTACK3_END );

    // Use either shooting or breath attack animation as ranged
    if ( info.animationLength[Bin_Info::SHOOT2] > 0 ) {
        appendFrames( info, _ranged[ATTACK_DIRECTION::TOP].start, Bin_Info::SHOOT1, true );
        appendFrames( info, _ranged[ATTACK_DIRECTION::TOP].end, Bin_Info::SHOOT1_END );

        appendFrames( info, _ranged[ATTACK_DIRECTION::FRONT].start, Bin_Info::SHOOT2, true );
        appendFrames( info, _ranged[ATTACK_DIRECTION::FRONT].end, Bin_Info::SHOOT2_END );

        appendFrames( info, _ranged[ATTACK_DIRECTION::BOTTOM].start, Bin_Info::SHOOT3, true );
        appendFrames( info, _ranged[ATTACK_DIRECTION::BOTTOM].end, Bin_Info::SHOOT3_END );
    }
    else if ( info.animationLength[Bin_Info::BREATH2] > 0 ) {
        // Only 6 units should have this
        appendFrames( info, _ranged[ATTACK_DIRECTION::TOP].start, Bin_Info::BREATH1, true );
        appendFrames( info, _ranged[ATTACK_DIRECTION::TOP].end, Bin_Info::BREATH1_END );

        appendFrames( info, _ranged[ATTACK_DIRECTION::FRONT].start, Bin_Info::BREATH2, true );
        appendFrames( info, _ranged[ATTACK_DIRECTION::FRONT].end, Bin_Info::BREATH2_END );

        appendFrames( info, _ranged[ATTACK_DIRECTION::BOTTOM].start, Bin_Info::BREATH3, true );
        appendFrames( info, _ranged[ATTACK_DIRECTION::BOTTOM].end, Bin_Info::BREATH3_END );
    }
}


AnimationReference::AnimationReference( const std::map<int, std::vector<int> > & animMap, int id )
{
    _type = id;

    // STATIC is our default
    // appendFrames inserts to vector so ref is still valid
    if ( !appendFrames( animMap, _static, Bin_Info::STATIC ) ) {
        // fall back to this, to avoid crashes
        _static.push_back( 1 );
    }

    // Taking damage
    appendFrames( animMap, _wince, Bin_Info::WINCE_UP );
    appendFrames( animMap, _wince, Bin_Info::WINCE_END ); // play it back together for now
    appendFrames( animMap, _death, Bin_Info::DEATH, true );

    // Idle animations
    for ( int idx = Bin_Info::IDLE1; idx <= Bin_Info::IDLE5; ++idx ) {
        std::vector<int> idleAnim;
        if ( appendFrames( animMap, idleAnim, idx ) ) {
            _idle.push_back( idleAnim );
        }
    }

    // Movement sequences
    // Every unit has MOVE_MAIN anim, use it as a base
    appendFrames( animMap, _loopMove, Bin_Info::MOVE_MAIN, true );

    
    if ( animMap.find( Bin_Info::MOVE_ONE ) != animMap.end() ) {
        appendFrames( animMap, _quickMove, Bin_Info::MOVE_ONE, true );
    }
    else {
        // this must be LICH or POWER_LICH
        _quickMove = _loopMove;
    }

    appendFrames( animMap, _moveModes.start, Bin_Info::MOVE_START );
    appendFrames( animMap, _moveModes.end, Bin_Info::MOVE_STOP );

    // Attack sequences
    appendFrames( animMap, _melee[ATTACK_DIRECTION::TOP].start, Bin_Info::ATTACK1, true );
    appendFrames( animMap, _melee[ATTACK_DIRECTION::TOP].end, Bin_Info::ATTACK1_END );

    appendFrames( animMap, _melee[ATTACK_DIRECTION::FRONT].start, Bin_Info::ATTACK2, true );
    appendFrames( animMap, _melee[ATTACK_DIRECTION::FRONT].end, Bin_Info::ATTACK2_END );

    appendFrames( animMap, _melee[ATTACK_DIRECTION::BOTTOM].start, Bin_Info::ATTACK3, true );
    appendFrames( animMap, _melee[ATTACK_DIRECTION::BOTTOM].end, Bin_Info::ATTACK3_END );

    // Use either shooting or breath attack animation as ranged
    if ( animMap.find(Bin_Info::SHOOT2) != animMap.end() ) {
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].start, Bin_Info::SHOOT1, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].end, Bin_Info::SHOOT1_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].start, Bin_Info::SHOOT2, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].end, Bin_Info::SHOOT2_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].start, Bin_Info::SHOOT3, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].end, Bin_Info::SHOOT3_END );
    }
    else if ( animMap.find( Bin_Info::BREATH2 ) != animMap.end() ) {
        // Only 6 units should have this
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].start, Bin_Info::BREATH1, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::TOP].end, Bin_Info::BREATH1_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].start, Bin_Info::BREATH2, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::FRONT].end, Bin_Info::BREATH2_END );

        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].start, Bin_Info::BREATH3, true );
        appendFrames( animMap, _ranged[ATTACK_DIRECTION::BOTTOM].end, Bin_Info::BREATH3_END );
    }
}

AnimationReference::~AnimationReference() {}


bool AnimationReference::appendFrames( const Bin_Info::MonsterAnimInfo & info, std::vector<int> & target, int animID, bool critical )
{
    // Sanity check
    if ( info.animationLength[animID] > 0 ) {
        for ( int i = 0; i < info.animationLength[animID]; i++ ) {
            target.push_back( info.animationFrames[animID][i] );
        }
        return true;
    } 
    else if ( critical ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "Monster type " << _type << ", " << info.animationLength[animID] << " frames when expected some" );
    }
    return false;
}

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
    case ANIMATION_TYPE::STATIC:
        return _static;
        break;
    case ANIMATION_TYPE::IDLE:
        return _idle.front();
        break;
    case ANIMATION_TYPE::MOVE_START:
        return _moveModes.start;
        break;
    case ANIMATION_TYPE::MOVING:
    case ANIMATION_TYPE::MOVE:
        return _loopMove;
        break;
    case ANIMATION_TYPE::MOVE_END:
        return _moveModes.end;
        break;
    case ANIMATION_TYPE::MOVE_QUICK:
        return _quickMove;
        break;
    case ANIMATION_TYPE::MELEE_TOP:
        return _melee[ATTACK_DIRECTION::TOP].start;
        break;
    case ANIMATION_TYPE::MELEE_TOP_END:
        return _melee[ATTACK_DIRECTION::TOP].end;
        break;
    case ANIMATION_TYPE::MELEE_FRONT:
        return _melee[ATTACK_DIRECTION::FRONT].start;
        break;
    case ANIMATION_TYPE::MELEE_FRONT_END:
        return _melee[ATTACK_DIRECTION::FRONT].end;
        break;
    case ANIMATION_TYPE::MELEE_BOT:
        return _melee[ATTACK_DIRECTION::BOTTOM].start;
        break;
    case ANIMATION_TYPE::MELEE_BOT_END:
        return _melee[ATTACK_DIRECTION::BOTTOM].end;
        break;
    case ANIMATION_TYPE::RANG_TOP:
        return _ranged[ATTACK_DIRECTION::TOP].start;
        break;
    case ANIMATION_TYPE::RANG_TOP_END:
        return _ranged[ATTACK_DIRECTION::TOP].end;
        break;
    case ANIMATION_TYPE::RANG_FRONT:
        return _ranged[ATTACK_DIRECTION::FRONT].start;
        break;
    case ANIMATION_TYPE::RANG_FRONT_END:
        return _ranged[ATTACK_DIRECTION::FRONT].end;
        break;
    case ANIMATION_TYPE::RANG_BOT:
        return _ranged[ATTACK_DIRECTION::BOTTOM].start;
        break;
    case ANIMATION_TYPE::RANG_BOT_END:
        return _ranged[ATTACK_DIRECTION::BOTTOM].end;
        break;
    case ANIMATION_TYPE::WNCE:
        return _wince;
        break;
    case ANIMATION_TYPE::KILL:
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
