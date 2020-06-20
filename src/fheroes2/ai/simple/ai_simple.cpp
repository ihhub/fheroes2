/********************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>               *
 *   All rights reserved.                                                       *
 *                                                                              *
 *   Part of the Free Heroes2 Engine:                                           *
 *   http://sourceforge.net/projects/fheroes2                                   *
 *                                                                              *
 *   Redistribution and use in source and binary forms, with or without         *
 *   modification, are permitted provided that the following conditions         *
 *   are met:                                                                   *
 *   - Redistributions may not be sold, nor may they be used in a               *
 *     commercial product or activity.                                          *
 *   - Redistributions of source code and/or in binary form must reproduce      *
 *     the above copyright notice, this list of conditions and the              *
 *     following disclaimer in the documentation and/or other materials         *
 *     provided with the distribution.                                          *
 *                                                                              *
 * THIS SOFTWARE IS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,   *
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,        *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE         *
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,            *
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           *
 *******************************************************************************/

#include <algorithm>

#include "ai_simple.h"
#include "castle.h"
#include "heroes.h"
#include "settings.h"

namespace AI
{
    Simple::Simple()
        : Base()
        , _kingdoms( KINGDOMMAX + 1 )
        , _heroes( HEROESMAXCOUNT + 2 )
    {}

    AIKingdom & Simple::GetKingdom( int color )
    {
        return _kingdoms.at( Color::GetIndex( color ) );
    }

    AIHero & Simple::GetHero( const Heroes & hero )
    {
        return _heroes.at( hero.GetID() );
    }

    const char * Simple::Type( void ) const
    {
        return "simple";
    }

    const char * Simple::License( void ) const
    {
        return "Non-Commercial";
    }

    void Simple::Reset( void )
    {
        for ( std::vector<AIKingdom>::iterator it = _kingdoms.begin(); it != _kingdoms.end(); ++it ) {
            it->Reset();
        }
        for ( std::vector<AIHero>::iterator it = _heroes.begin(); it != _heroes.end(); ++it ) {
            it->Reset();
        }
    }

    AIHero::AIHero()
        : primary_target( -1 )
        , fix_loop( 0 )
    {}

    void AIHero::ClearTasks( void )
    {
        sheduled_visit.clear();
    }

    void AIHero::Reset( void )
    {
        primary_target = -1;
        sheduled_visit.clear();
        fix_loop = 0;
    }

    void AIKingdom::Reset( void )
    {
        capital = NULL;
        scans.clear();
    }

    bool Queue::isPresent( s32 index ) const
    {
        for ( const_iterator it = begin(); it != end(); ++it ) {
            if ( it->first == index )
                return true;
        }
        return false;
    }

    void IndexObjectMap::DumpObjects( const IndexDistance & id )
    {
        IndexObjectMap::const_iterator it = find( id.first );

        if ( it != end() )
            DEBUG( DBG_AI, DBG_TRACE, MP2::StringObject( ( *it ).second ) << ", maps index: " << id.first << ", dist: " << id.second );
    }
}
