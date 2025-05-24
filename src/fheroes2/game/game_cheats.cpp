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

#include "army_troop.h"
#include "castle.h"
#include "game_interface.h"
#include "game_over.h"
#include "heroes.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "logging.h"
#include "monster.h"
#include "resource.h"
#include "settings.h"
#include "spell.h"
#include "spell_storage.h"
#include "world.h"

namespace GameCheats
{
    namespace
    {
        std::string buffer;
        bool enabled = false;

        const size_t MAX_BUFFER = 32;

        void checkBuffer()
        {
            const Settings & conf = Settings::Get();

            if ( buffer.find( "19960214" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: resources" );
                Kingdom & kingdom = World::Get().GetKingdom( conf.CurrentColor() );
                kingdom.AddFundsResource( Funds( 0, 0, 0, 0, 0, 0, 999999 ) );
                kingdom.AddFundsResource( Funds( Resource::WOOD, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::ORE, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::MERCURY, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::SULFUR, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::CRYSTAL, 999 ) );
                kingdom.AddFundsResource( Funds( Resource::GEMS, 999 ) );
                buffer.clear();
            }

            else if ( buffer.find( "8675309" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: reveal all fog" );
                World::Get().RevealMap( conf.CurrentColor() );
                Interface::GameArea::updateMapFogDirections();
                buffer.clear();
            }

            else if ( buffer.find( "32167" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: black dragons" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    hero->GetArmy().JoinTroop( Monster::BLACK_DRAGON, 5, true );
                }
                buffer.clear();
            }
            else if ( buffer.find( "24680" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: upgraded army" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    const int race = hero->GetRace();
                    const uint32_t dwellings[] = { DWELLING_UPGRADE2, DWELLING_UPGRADE3, DWELLING_UPGRADE4, DWELLING_UPGRADE5, DWELLING_UPGRADE7 };

                    Army & army = hero->GetArmy();
                    for ( size_t i = 0; i < Army::maximumTroopCount; ++i ) {
                        Troop * troop = army.GetTroop( i );
                        if ( troop && troop->isValid() ) {
                            bool keep = false;
                            for ( const uint32_t dw : dwellings ) {
                                const Monster m( race, dw );
                                if ( troop->isMonster( m.GetID() ) ) {
                                    keep = true;
                                    break;
                                }
                            }
                            if ( !keep )
                                troop->Reset();
                        }
                    }

                    for ( const uint32_t dw : dwellings ) {
                        const Monster monster( race, dw );
                        if ( monster.isValid() )
                            army.JoinTroop( monster, 5, true );
                    }
                }
                buffer.clear();
            }
            else if ( buffer.find( "13579" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: random troop" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    const Monster monster = Monster::Rand( Monster::LevelType::LEVEL_ANY );
                    hero->GetArmy().JoinTroop( monster, 5, true );
                }
                buffer.clear();
            }
            else if ( buffer.find( "12345" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: max primary skills" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    hero->setAttackBaseValue( 20 );
                    hero->setDefenseBaseValue( 20 );
                    hero->setPowerBaseValue( 20 );
                    hero->setKnowledgeBaseValue( 20 );
                }
                buffer.clear();
            }
            else if ( buffer.find( "654321" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: max secondary skills" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    for ( int skill = 1; skill <= Skill::Secondary::ESTATES; ++skill ) {
                        hero->LearnSkill( Skill::Secondary( skill, Skill::Level::EXPERT ) );
                    }
                }
                buffer.clear();
            }
            else if ( buffer.find( "11111" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: infinite movement" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    hero->IncreaseMovePoints( hero->GetMaxMovePoints() * 10 );
                }
                buffer.clear();
            }
            else if ( buffer.find( "22222" ) != std::string::npos ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Cheat activated: all spells" );
                if ( Heroes * hero = Interface::GetFocusHeroes() ) {
                    SpellStorage storage;
                    for ( int spellId : Spell::getAllSpellIdsSuitableForSpellBook() ) {
                        storage.Append( Spell( spellId ) );
                    }
                    hero->AppendSpellsToBook( storage, true );
                    hero->SetSpellPoints( hero->GetMaxSpellPoints() );
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
