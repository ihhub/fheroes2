/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "monster_anim.h"
#include "monster.h"
#include "rand.h"

namespace fheroes2
{
    RandomMonsterAnimation::RandomMonsterAnimation( const Monster & monster )
        : _reference( monster.GetID() )
        , _icnID( fheroes2::getMonsterData( monster.GetID() ).icnId )
        , _frameId( 0 )
        , _frameOffset( 0 )
        , _isFlyer( monster.isFlying() )
    {
        _addValidMove( Monster_Info::STATIC );
        _addValidMove( Monster_Info::STATIC );
        _addValidMove( Monster_Info::IDLE );
        _addValidMove( Monster_Info::MELEE_TOP );
        _addValidMove( Monster_Info::MELEE_FRONT );
        _addValidMove( Monster_Info::MELEE_BOT );
        _addValidMove( Monster_Info::RANG_TOP );
        _addValidMove( Monster_Info::RANG_FRONT );
        _addValidMove( Monster_Info::RANG_BOT );
        _addValidMove( Monster_Info::MOVING );
        _addValidMove( Monster_Info::MOVING );
        _addValidMove( Monster_Info::WNCE );
        _addValidMove( Monster_Info::KILL );

        increment();
    }

    void RandomMonsterAnimation::increment()
    {
        if ( _frameSet.empty() ) {
            // make sure both are empty to avoid leftovers in case of mismatch
            _offsetSet.clear();

            const int moveId = Rand::Get( _validMoves );

            if ( moveId == Monster_Info::STATIC ) {
                const u32 counter = Rand::Get( 10, 20 );
                for ( u32 i = 0; i < counter; ++i )
                    _pushFrames( Monster_Info::STATIC );
            }
            else if ( moveId == Monster_Info::IDLE ) {
                _pushFrames( Monster_Info::IDLE );
            }
            else if ( moveId == Monster_Info::MOVING ) {
                _pushFrames( _isFlyer ? Monster_Info::FLY_UP : Monster_Info::MOVE_START );

                const u32 counter = Rand::Get( 3, 5 );
                for ( u32 j = 0; j < counter; ++j )
                    _pushFrames( Monster_Info::MOVING );

                _pushFrames( _isFlyer ? Monster_Info::FLY_LAND : Monster_Info::MOVE_END );
            }
            else if ( moveId == Monster_Info::MELEE_TOP ) {
                _pushFrames( Monster_Info::MELEE_TOP );
                _pushFrames( Monster_Info::MELEE_TOP_END );
            }
            else if ( moveId == Monster_Info::MELEE_FRONT ) {
                _pushFrames( Monster_Info::MELEE_FRONT );
                _pushFrames( Monster_Info::MELEE_FRONT_END );
            }
            else if ( moveId == Monster_Info::MELEE_BOT ) {
                _pushFrames( Monster_Info::MELEE_BOT );
                _pushFrames( Monster_Info::MELEE_BOT_END );
            }
            else if ( moveId == Monster_Info::RANG_TOP ) {
                _pushFrames( Monster_Info::RANG_TOP );
                _pushFrames( Monster_Info::RANG_TOP_END );
            }
            else if ( moveId == Monster_Info::RANG_FRONT ) {
                _pushFrames( Monster_Info::RANG_FRONT );
                _pushFrames( Monster_Info::RANG_FRONT_END );
            }
            else if ( moveId == Monster_Info::RANG_BOT ) {
                _pushFrames( Monster_Info::RANG_BOT );
                _pushFrames( Monster_Info::RANG_BOT_END );
            }
            else if ( moveId == Monster_Info::WNCE ) {
                _pushFrames( Monster_Info::WNCE );
            }
            else if ( moveId == Monster_Info::KILL ) {
                _pushFrames( Monster_Info::KILL );
            }

            _pushFrames( Monster_Info::STATIC );
        }

        _updateFrameInfo();
    }

    int RandomMonsterAnimation::icnFile() const
    {
        return _icnID;
    }

    int RandomMonsterAnimation::frameId() const
    {
        return _frameId;
    }

    int RandomMonsterAnimation::offset() const
    {
        return _frameOffset;
    }

    void RandomMonsterAnimation::reset()
    {
        _frameSet.clear();
        _offsetSet.clear();

        _pushFrames( Monster_Info::STATIC );
        _updateFrameInfo();
    }

    void RandomMonsterAnimation::_pushFrames( const Monster_Info::AnimationType type )
    {
        const std::vector<int> & sequence = _reference.getAnimationVector( type );
        _frameSet.insert( _frameSet.end(), sequence.begin(), sequence.end() );

        if ( type == Monster_Info::IDLE ) { // a special case
            _offsetSet.insert( _offsetSet.end(), sequence.size(), 0 );
        }
        else {
            const std::vector<int> & offset = _reference.getAnimationOffset( type );
            _offsetSet.insert( _offsetSet.end(), offset.begin(), offset.end() );
        }

        if ( _offsetSet.size() != _frameSet.size() )
            _offsetSet.resize( _frameSet.size(), 0 );
    }

    void RandomMonsterAnimation::_addValidMove( const Monster_Info::AnimationType type )
    {
        if ( !_reference.getAnimationVector( type ).empty() )
            _validMoves.push_back( type );
    }

    void RandomMonsterAnimation::_updateFrameInfo()
    {
        if ( _frameSet.empty() )
            return;

        _frameId = _frameSet.front();
        _frameSet.pop_front();

        if ( !_offsetSet.empty() ) {
            _frameOffset = _offsetSet.front();
            _offsetSet.pop_front();
        }
    }
}
