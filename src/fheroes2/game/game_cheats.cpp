/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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
#include "game_cheats.h"

#include <SDL2/SDL.h>
#include "logging.h"
#include "settings.h"
#include "world.h"
#include "kingdom.h"
#include "resource.h"
#include "game_interface.h"
#include "heroes.h"
#include "army.h"
#include "monster.h"

namespace GameCheats
{
    namespace
    {
        std::string buffer;
        bool enabled = false;

        const size_t MAX_BUFFER = 32;

        constexpr const char * CHEAT_SHOWMAP = "12345";
        constexpr const char * CHEAT_RESOURCES = "67890";
        constexpr const char * CHEAT_BLACKDRAGON = "32167";

        void checkBuffer()
        {
            if ( buffer.find( CHEAT_SHOWMAP ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: showmap" );
                World::Get().ClearFog( Settings::Get().CurrentColor() );
                buffer.clear();
            }
            else if ( buffer.find( CHEAT_RESOURCES ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: resources" );
                Kingdom & kingdom = World::Get().GetKingdom( Settings::Get().CurrentColor() );
                kingdom.AddFundsResource( Funds( 0, 0, 0, 0, 0, 0, 10000 ) );
                kingdom.AddFundsResource( Funds( Resource::WOOD, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::ORE, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::MERCURY, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::SULFUR, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::CRYSTAL, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::GEMS, 999 ) );
                buffer.clear();
            }
            else if ( buffer.find( CHEAT_BLACKDRAGON ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: black dragons" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    hero->GetArmy().JoinTroop( Monster::BLACK_DRAGON, 2, false );
                }
                buffer.clear();
            }
        }
    }

    void enableCheats( bool enable )
    {
        enabled = enable;
        if ( !enable )
            buffer.clear();
    }

    bool cheatsEnabled()
    {
        return enabled;
    }

    void reset()
    {
        buffer.clear();
    }

    void onKeyPressed( const fheroes2::Key key, const int32_t modifier )
    {
        if ( !enabled || SDL_IsTextInputActive() )
            return;

        std::string tmp;
        size_t pos = 0;
        pos = fheroes2::InsertKeySym( tmp, pos, key, modifier );
        if ( pos == 0 || tmp.empty() )
            return;

        buffer += tmp;
        if ( buffer.size() > MAX_BUFFER )
            buffer.erase( 0, buffer.size() - MAX_BUFFER );

        checkBuffer();
    }
}

