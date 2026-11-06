/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "multi_reservation.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <optional>
#include <sstream>
#include <thread>
#include <vector>

#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "multi_connection.h"
#include "multi_game.h"
#include "multi_live.h"
#include "multi_live_battle.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "multi_notice.h"
#include "multi_protocol.h"
#include "multi_simultaneous.h"
#include "players.h"
#include "route.h"
#include "translations.h"
#include "world.h"

namespace
{
    // The step is refused when there is no answer after this time.
    constexpr std::chrono::seconds replyTimeout{ 15 };

    void insertAround( Multi::ReservationZone & zone, const int32_t index )
    {
        if ( !Maps::isValidAbsIndex( index ) ) {
            return;
        }

        const fheroes2::Point center = Maps::GetPoint( index );

        for ( int32_t dy = -1; dy <= 1; ++dy ) {
            for ( int32_t dx = -1; dx <= 1; ++dx ) {
                if ( !Maps::isValidAbsPoint( center.x + dx, center.y + dy ) ) {
                    continue;
                }

                const int32_t tile = Maps::GetIndexFromAbsPoint( center.x + dx, center.y + dy );
                if ( !zone.tiles.contains( tile ) ) {
                    zone.tiles.insert( tile );
                    zone.tileList.push_back( tile );
                }
            }
        }
    }

    // A battle takes one hero, or one castle with the hero in it.
    bool canBeVanquishedByOneBattle( const Kingdom & kingdom )
    {
        return kingdom.GetCastles().size() + kingdom.GetHeroes().size() <= 2;
    }

    // A vanquished player loses everything it captured on the map.
    void insertCapturedObjects( Multi::ReservationZone & zone, const PlayerColor color )
    {
        for ( int32_t index = 0; index < static_cast<int32_t>( world.getSize() ); ++index ) {
            if ( MP2::isCaptureObject( world.getTile( index ).getMainObjectType( false ) ) && world.ColorCapturedObject( index ) == color ) {
                insertAround( zone, index );
            }
        }
    }

    // The computer holding a tile or a kingdom today, its number plus one. 0 for nobody.
    using Holder = uint8_t;

    constexpr Holder noHolder{ 0 };

    Holder getHolder( const Multi::MachineId machine )
    {
        return static_cast<Holder>( machine + 1 );
    }

    // Who holds what today. A guest only remembers what it got.
    class ReservationBook
    {
    public:
        void prepare( const uint32_t day )
        {
            if ( _day == day && _tiles.size() == world.getSize() ) {
                return;
            }

            _day = day;
            _tiles.assign( world.getSize(), noHolder );
            _kingdoms.fill( noHolder );
        }

        // All or nothing. takenTile is the first tile held by the other one, or -1 if a player is taken.
        bool reserve( const std::vector<int32_t> & tiles, const PlayerColorsSet kingdoms, const Holder holder, int32_t & takenTile )
        {
            takenTile = -1;

            for ( const int32_t tile : tiles ) {
                if ( tile < 0 || static_cast<size_t>( tile ) >= _tiles.size() ) {
                    return false;
                }
                if ( _tiles[tile] != noHolder && _tiles[tile] != holder ) {
                    takenTile = tile;
                    return false;
                }
            }

            for ( const PlayerColor color : PlayerColorsVector( kingdoms ) ) {
                const Holder current = _kingdoms[Color::GetIndex( color )];
                if ( current != noHolder && current != holder ) {
                    return false;
                }
            }

            for ( const int32_t tile : tiles ) {
                _tiles[tile] = holder;
            }
            for ( const PlayerColor color : PlayerColorsVector( kingdoms ) ) {
                _kingdoms[Color::GetIndex( color )] = holder;
            }

            return true;
        }

        // Retunrs the tiles which are not held yet by this holder.
        std::vector<int32_t> missingTiles( const std::vector<int32_t> & tiles, const Holder holder ) const
        {
            std::vector<int32_t> result;

            for ( const int32_t tile : tiles ) {
                if ( tile >= 0 && static_cast<size_t>( tile ) < _tiles.size() && _tiles[tile] != holder ) {
                    result.push_back( tile );
                }
            }

            return result;
        }

        PlayerColorsSet missingKingdoms( const PlayerColorsSet kingdoms, const Holder holder ) const
        {
            PlayerColorsSet result = 0;

            for ( const PlayerColor color : PlayerColorsVector( kingdoms ) ) {
                if ( _kingdoms[Color::GetIndex( color )] != holder ) {
                    result |= static_cast<PlayerColorsSet>( color );
                }
            }

            return result;
        }

    private:
        uint32_t _day{ 0 };
        std::vector<Holder> _tiles;
        std::array<Holder, maxNumOfPlayers> _kingdoms{};
    };

    struct ReservationState
    {
        bool isEnabled{ false };
        bool isTestJumpsEnabled{ false };

        ReservationBook book;
        uint32_t nextRequestId{ 1 };

        // Last checked step. A step is reserved only once while the hero walks.
        int32_t heroId{ -1 };
        int32_t from{ -1 };
        int32_t target{ -1 };
        bool isAllowed{ true };

        Multi::ReservationStats stats;
    };

    ReservationState & reservationState()
    {
        static ReservationState state;
        return state;
    }

    // Asks the host and waits for its answer.
    bool askHost( const std::vector<int32_t> & tiles, const PlayerColorsSet kingdoms )
    {
        ReservationState & state = reservationState();

        Multi::NetworkSession * session = Multi::getSession();
        if ( session == nullptr || !session->isAlive() ) {
            return false;
        }

        Multi::ReserveRequest request;
        request.requestId = state.nextRequestId++;
        request.day = world.CountDay();
        request.kingdoms = kingdoms;
        request.tiles = tiles;

        session->send( Multi::hostMachine, Multi::makeReserveRequest( request ) );
        ++state.stats.requestsSent;

        auto start = std::chrono::steady_clock::now();

        while ( std::chrono::steady_clock::now() - start < replyTimeout ) {
            session->update();

            if ( std::optional<Multi::Message> message = session->receive( Multi::MessageType::RESERVE_REPLY ); message ) {
                const std::optional<Multi::ReserveReply> reply = Multi::readReserveReply( *message );

                // Late answer to an earlier request, already refused.
                if ( reply && reply->requestId < request.requestId ) {
                    continue;
                }

                if ( !reply || reply->requestId != request.requestId ) {
                    ERROR_LOG( "[MULTI] Unexpected reservation answer" )
                    return false;
                }

                const uint32_t milliseconds
                    = static_cast<uint32_t>( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - start ).count() );
                state.stats.totalWaitMilliseconds += milliseconds;
                state.stats.maxWaitMilliseconds = std::max( state.stats.maxWaitMilliseconds, milliseconds );

                return reply->isGranted;
            }

            // The request waits while the connection is lost. That time does not count.
            if ( session->state() != Multi::NetworkSession::State::CONNECTED ) {
                if ( !Multi::keepConnection() ) {
                    return false;
                }

                start = std::chrono::steady_clock::now();
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }

        ERROR_LOG( "[MULTI] No answer from the host for a reservation" )
        return false;
    }

    bool reserve( const Multi::ReservationZone & zone )
    {
        ReservationState & state = reservationState();
        state.book.prepare( world.CountDay() );

        const Holder holder = getHolder( Multi::getThisMachine() );

        if ( !Multi::isGuest() ) {
            int32_t takenTile = -1;
            return state.book.reserve( zone.tileList, zone.kingdoms, holder, takenTile );
        }

        const std::vector<int32_t> tiles = state.book.missingTiles( zone.tileList, holder );
        const PlayerColorsSet kingdoms = state.book.missingKingdoms( zone.kingdoms, holder );

        if ( tiles.empty() && kingdoms == 0 ) {
            return true;
        }

        if ( !askHost( tiles, kingdoms ) ) {
            return false;
        }

        int32_t takenTile = -1;
        state.book.reserve( tiles, kingdoms, holder, takenTile );

        return true;
    }
}

Multi::ReservationZone Multi::getStepZone( const Heroes & hero, const int32_t targetIndex, const PlayerColorsSet otherComputerColors )
{
    ReservationZone zone;
    zone.tiles = TileSet( static_cast<int32_t>( world.getSize() ) );

    // The tiles a step changes are next to where the hero comes from or goes to.
    insertAround( zone, hero.GetIndex() );
    insertAround( zone, targetIndex );

    // Stone liths and whirlpools send the hero to one of their exits, picked at random by the game.
    if ( Maps::isValidAbsIndex( targetIndex ) ) {
        MapsIndexes exits = world.GetTeleportEndPoints( targetIndex );
        if ( exits.empty() ) {
            exits = world.GetWhirlpoolEndPoints( targetIndex );
        }

        for ( const int32_t exitIndex : exits ) {
            insertAround( zone, exitIndex );
        }
    }

    PlayerColorsSet battleColors = 0;

    for ( const int32_t index : zone.tileList ) {
        const Maps::Tile & tile = world.getTile( index );

        // There may be a battle with a hero or a castle of another player. A captured object only changes its own tile.
        if ( const Heroes * other = tile.getHero(); other != nullptr && other != &hero ) {
            battleColors |= static_cast<PlayerColorsSet>( other->GetColor() );
        }
        if ( const Castle * castle = world.getCastle( Maps::GetPoint( index ) ); castle != nullptr ) {
            battleColors |= static_cast<PlayerColorsSet>( castle->GetColor() );
        }
    }

    for ( const PlayerColor owner : PlayerColorsVector( static_cast<PlayerColorsSet>( battleColors & ~static_cast<PlayerColorsSet>( hero.GetColor() ) ) ) ) {
        // An attack on a player of another computer is arranged with that computer.
        if ( ( otherComputerColors & owner ) != 0 ) {
            continue;
        }

        zone.kingdoms |= static_cast<PlayerColorsSet>( owner );

        if ( canBeVanquishedByOneBattle( world.GetKingdom( owner ) ) ) {
            insertCapturedObjects( zone, owner );
        }
    }

    // The last hero of a player without castle may lose a battle too: the player is vanquished at once.
    const Kingdom & ownKingdom = world.GetKingdom( hero.GetColor() );
    if ( ownKingdom.GetCastles().empty() && ownKingdom.GetHeroes().size() <= 1 ) {
        insertCapturedObjects( zone, hero.GetColor() );
    }

    zone.isBattleWithOtherComputer = attacksOtherComputer( hero, targetIndex, otherComputerColors );

    return zone;
}

bool Multi::attacksOtherComputer( const Heroes & hero, const int32_t targetIndex, const PlayerColorsSet otherComputerColors )
{
    if ( !Maps::isValidAbsIndex( targetIndex ) ) {
        return false;
    }

    const auto isEnemyOfOtherComputer
        = [&hero, otherComputerColors]( const PlayerColor color ) { return ( otherComputerColors & color ) != 0 && !hero.isFriends( color ); };

    if ( const Heroes * other = world.getTile( targetIndex ).getHero(); other != nullptr && other != &hero && isEnemyOfOtherComputer( other->GetColor() ) ) {
        return true;
    }

    const Castle * castle = world.getCastleEntrance( Maps::GetPoint( targetIndex ) );
    return castle != nullptr && isEnemyOfOtherComputer( castle->GetColor() );
}

void Multi::enableReservations()
{
    reservationState().isEnabled = true;
}

bool Multi::areReservationsEnabled()
{
    return reservationState().isEnabled || isSimultaneousDay();
}

void Multi::reserveCastlesForTheDay()
{
    ReservationState & state = reservationState();
    state.book.prepare( world.CountDay() );

    const auto reserveCastles = [&state]( const PlayerColorsSet colors, const Holder holder ) {
        for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
            for ( const Castle * castle : world.GetKingdom( color ).GetCastles() ) {
                TileSet tiles( static_cast<int32_t>( world.getSize() ) );
                insertCastleFootprint( tiles, castle->GetCenter() );

                std::vector<int32_t> tileList;
                for ( int32_t index = 0; index < static_cast<int32_t>( world.getSize() ); ++index ) {
                    if ( tiles.contains( index ) ) {
                        tileList.push_back( index );
                    }
                }

                int32_t takenTile = -1;
                state.book.reserve( tileList, 0, holder, takenTile );
            }
        }
    };

    // Each computer holds the castles of its players.
    for ( const MachineInfo & info : getMachineTable().machines ) {
        reserveCastles( info.playedColors, getHolder( info.id ) );
    }
}

namespace
{
    void showRefusal( const Heroes & hero, const bool isBattle )
    {
        if ( !hero.isControlHuman() ) {
            return;
        }

        const char * text = isBattle ? gettext_noop( "No battle between players during simultaneous turns. Tomorrow, each player will play in turn." )
                                     : gettext_noop( "Another player has already been here. This place is theirs until tomorrow." );
        Multi::showMapNote( _( text ) );
    }
}

bool Multi::onHeroMove( Heroes & hero, const bool isJump )
{
    ReservationState & state = reservationState();

    // AI players play after the human players, on the host.
    if ( !areReservationsEnabled() || !isNetworkGame() || !hero.GetPath().isValidForMovement() || ( Players::HumanColors() & hero.GetColor() ) == 0 ) {
        return true;
    }

    // Check only when the hero is about to walk to the next tile.
    if ( !isJump && hero.GetSpriteIndex() < 45 && ( hero.GetSpriteIndex() % Heroes::heroFrameCountPerTile ) > 1 ) {
        return true;
    }

    const int32_t target = hero.GetPath().GetFrontIndex();
    const bool isNewStep = ( state.heroId != hero.GetID() || state.from != hero.GetIndex() || state.target != target );
    bool isBattle = false;
    bool isRefusalShown = false;

    if ( isNewStep ) {
        state.heroId = hero.GetID();
        state.from = hero.GetIndex();
        state.target = target;

        const ReservationZone zone = getStepZone( hero, target, getRemoteHumanColors() );
        isBattle = zone.isBattleWithOtherComputer;

        if ( isBattle && isSimultaneousDay() ) {
            // The battle is played now. requestLiveBattle() tells the player if it cannot be.
            state.isAllowed = requestLiveBattle( hero, target );
            isRefusalShown = true;

            if ( !state.isAllowed ) {
                ++state.stats.battlesRefused;
            }
        }
        else if ( isBattle ) {
            ++state.stats.battlesRefused;
            state.isAllowed = false;
            askClassicDay();
        }
        else {
            state.isAllowed = reserve( zone );
            if ( state.isAllowed ) {
                ++state.stats.stepsReserved;
            }
            else {
                ++state.stats.stepsRefused;
            }
        }
    }

    if ( state.isAllowed ) {
        return true;
    }

    hero.GetPath().Reset();
    hero.SetMove( false );

    if ( !hero.isControlHuman() ) {
        // A hero controlled by AI would otherwise try again and again.
        hero.ResetMovePoints();
    }
    else if ( isNewStep && !isRefusalShown ) {
        showRefusal( hero, isBattle );
    }

    return false;
}

bool Multi::reserveSpellTarget( const Heroes & hero, const int32_t targetIndex )
{
    ReservationState & state = reservationState();

    if ( !areReservationsEnabled() || !isNetworkGame() || ( Players::HumanColors() & hero.GetColor() ) == 0 ) {
        return true;
    }

    // The next step must be treated as a new one, wherever the hero lands.
    state.heroId = -1;

    const ReservationZone zone = getStepZone( hero, targetIndex, getRemoteHumanColors() );

    if ( zone.isBattleWithOtherComputer ) {
        ++state.stats.battlesRefused;
        askClassicDay();
        showRefusal( hero, true );
        return false;
    }

    if ( !reserve( zone ) ) {
        ++state.stats.stepsRefused;
        showRefusal( hero, false );
        return false;
    }

    ++state.stats.stepsReserved;
    return true;
}

void Multi::answerReservations()
{
    ReservationState & state = reservationState();

    NetworkSession * session = getSession();
    if ( !areReservationsEnabled() || isGuest() || session == nullptr ) {
        return;
    }

    session->update();

    while ( std::optional<Message> message = session->receive( MessageType::RESERVE ) ) {
        const std::optional<ReserveRequest> request = readReserveRequest( *message );

        ReserveReply reply;
        if ( request ) {
            reply.requestId = request->requestId;

            // A request for another day comes from a computer that is behind. Nothing is granted.
            if ( request->day == world.CountDay() ) {
                state.book.prepare( world.CountDay() );
                const PlayerColorsSet kingdoms = static_cast<PlayerColorsSet>( request->kingdoms & Color::allPlayerColors() );
                reply.isGranted = state.book.reserve( request->tiles, kingdoms, getHolder( message->sender ), reply.takenTile );
            }
        }
        else {
            ERROR_LOG( "[MULTI] Invalid reservation request received" )
        }

        if ( reply.isGranted ) {
            ++state.stats.requestsGranted;
        }
        else {
            ++state.stats.requestsRefused;
        }

        session->send( message->sender, makeReserveReply( reply ) );
    }
}

const Multi::ReservationStats & Multi::getReservationStats()
{
    return reservationState().stats;
}

void Multi::enableTestJumps()
{
    reservationState().isTestJumpsEnabled = true;
}

void Multi::jumpForTest( const PlayerColor color )
{
    ReservationState & state = reservationState();

    if ( !state.isTestJumpsEnabled || !isSimultaneousDay() || ( getLocalHumanColors() & color ) == 0 ) {
        return;
    }

    const VecHeroes & heroes = world.GetKingdom( color ).GetHeroes();
    if ( heroes.empty() || heroes.front()->isShipMaster() || heroes.front()->inCastle() != nullptr ) {
        return;
    }

    Heroes & hero = *heroes.front();
    const fheroes2::Point center = hero.GetCenter();

    // Use the same targets on every run of the same game.
    uint32_t mix = world.CountDay() * 2654435761U + static_cast<uint32_t>( hero.GetID() ) * 40503U;

    for ( int attempt = 0; attempt < 30; ++attempt ) {
        mix = mix * 1103515245U + 12345U;

        const int32_t x = center.x + static_cast<int32_t>( ( mix >> 8 ) % 9 ) - 4;
        const int32_t y = center.y + static_cast<int32_t>( ( mix >> 16 ) % 9 ) - 4;
        if ( !Maps::isValidAbsPoint( x, y ) ) {
            continue;
        }

        const int32_t target = Maps::GetIndexFromAbsPoint( x, y );
        if ( target == hero.GetIndex() || !Maps::isValidForDimensionDoor( target, false ) ) {
            continue;
        }

        if ( !reserveSpellTarget( hero, target ) ) {
            ++state.stats.testJumpsRefused;
            return;
        }

        hero.Scout( target );
        hero.Move2Dest( target );
        hero.GetPath().Reset();
        hero.ActionNewPosition( false );

        ++state.stats.testJumps;
        sendLiveUpdate( &hero );
        return;
    }
}

std::string Multi::describeReservations()
{
    const ReservationStats & stats = reservationState().stats;

    std::ostringstream os;
    os << stats.stepsReserved << " step(s) reserved, " << stats.stepsRefused << " refused, " << stats.battlesRefused << " battle(s) refused, " << stats.requestsSent
       << " request(s) sent (" << ( stats.requestsSent > 0 ? stats.totalWaitMilliseconds / stats.requestsSent : 0 ) << " ms on average, " << stats.maxWaitMilliseconds
       << " at most), " << stats.requestsGranted << " granted and " << stats.requestsRefused << " refused for the guests";

    if ( stats.testJumps > 0 || stats.testJumpsRefused > 0 ) {
        os << ", " << stats.testJumps << " test jump(s), " << stats.testJumpsRefused << " refused";
    }

    return os.str();
}
