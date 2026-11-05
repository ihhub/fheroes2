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

#include "multi_simultaneous.h"

#include <algorithm>
#include <map>
#include <optional>
#include <sstream>

#include "color.h"
#include "dialog.h"
#include "game_over.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "multi_contact.h"
#include "multi_game.h"
#include "multi_game_over.h"
#include "multi_live.h"
#include "multi_machines.h"
#include "multi_menu.h"
#include "multi_network.h"
#include "multi_notice.h"
#include "multi_protocol.h"
#include "multi_reservation.h"
#include "multi_save_parts.h"
#include "multi_spectator.h"
#include "multi_world_changes.h"
#include "players.h"
#include "serialize.h"
#include "spell.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

namespace
{
    struct Measure
    {
        Multi::SimultaneousStats stats;

        uint32_t lastDay{ 0 };
        bool isLastDayContact{ false };
    };

    Measure & measure()
    {
        static Measure value;
        return value;
    }

    // A player who wins or loses this morning must see it at the start of their turn.
    bool hasConditionMet( const PlayerColorsSet colors )
    {
        for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
            const Kingdom & kingdom = world.GetKingdom( color );

            if ( world.CheckKingdomWins( kingdom ) != GameOver::COND_NONE || world.CheckKingdomLoss( kingdom ) != GameOver::COND_NONE ) {
                return true;
            }
        }

        return false;
    }

    // For self-tests only, it shows hidden things.
    std::string describeVerdict( const Multi::ContactVerdict & verdict )
    {
        std::ostringstream os;

        if ( verdict.witnessTile != -1 ) {
            const fheroes2::Point point = Maps::GetPoint( verdict.witnessTile );
            os << ", witness tile [" << point.x << ", " << point.y << "] for " << Color::String( verdict.witnessColors[0] ) << " and "
               << Color::String( verdict.witnessColors[1] );
        }

        for ( const Multi::PlayerReach & reach : verdict.reaches ) {
            os << "\n  " << Color::String( reach.color ) << ": " << reach.heroSources << " hero(es), " << reach.hireSources << " castle(s) hiring, " << reach.rounds
               << " round(s), unlocked " << reach.castleUnlocks << " castle(s), " << reach.jailSources << " jail(s), " << reach.movementBonuses << " movement bonus(es)"
               << ( reach.isRoundLimitReached ? ", still growing" : "" );
        }

        return os.str();
    }

    struct DayState
    {
        bool isEnabled{ false };
        bool isActive{ false };
        uint32_t day{ 0 };
        bool isClassicDayAsked{ false };

        // Host only. A new day starts with its first human player.
        bool isDayStartPending{ false };

        // Setting changed by a player of another computer, shown at the next safe moment.
        std::optional<bool> notice;
        Multi::MachineId noticeFrom{ Multi::hostMachine };

        // End of the day. The players can resume their day until the host locks it.
        PlayerColorsSet playedColors{ 0 };
        bool isReadyHere{ false };
        bool isResumeAsked{ false };
        bool isResumeAccepted{ false };
        bool isDayDoneSent{ false };
        bool isLocked{ false };

        // The other computers whose players are done, and on the host the guests which sent their last changes.
        Multi::MachineSet doneMachines{ 0 };
        Multi::MachineSet closedMachines{ 0 };
        std::vector<Multi::MachineId> readyNotices;

        // The computers playing today, the host first. Each one offers its share of the free heroes.
        std::vector<Multi::MachineId> dayMachines;

        bool isTestToggleEnabled{ false };
        uint32_t lastTestToggleDay{ 0 };

        std::vector<Multi::AlliedVisit> alliedVisits;
        // Host only, the visits of each guest.
        std::map<Multi::MachineId, std::vector<Multi::AlliedVisit>> otherAlliedVisits;
        bool isMarkingAlliedVisits{ false };

        Multi::SimultaneousDayStats stats;
    };

    DayState & dayState()
    {
        static DayState state;
        return state;
    }

    // Host only. The guests which play today and have still something to do: gone ones and ones without players are not
    // waited for.
    Multi::MachineSet getAwaitedGuests()
    {
        Multi::MachineSet guests = 0;

        for ( const Multi::MachineId machine : dayState().dayMachines ) {
            const Multi::MachineInfo * info = Multi::findMachine( machine );
            if ( machine != Multi::hostMachine && info != nullptr && info->state != Multi::MachineState::GONE && Multi::hasPlayerInGame( info->playedColors ) ) {
                guests |= Multi::machineBit( machine );
            }
        }

        return guests;
    }

    // The guests can no longer resume their day. Their last changes must be applied before the turns of AI players.
    void lockDay()
    {
        DayState & state = dayState();
        if ( state.isLocked || Multi::isGuest() ) {
            return;
        }

        state.isLocked = true;
        Multi::sendToOthers( Multi::makeDayMessage( Multi::MessageType::DAY_LOCKED, state.day ) );

        Multi::waitForOtherDaysClosed();
    }

    void markAlliedVisits( const std::vector<Multi::AlliedVisit> & visits )
    {
        DayState & state = dayState();
        state.isMarkingAlliedVisits = true;

        for ( const Multi::AlliedVisit & visit : visits ) {
            if ( !Heroes::isValidId( visit.heroId ) || !Maps::isValidAbsIndex( visit.tileIndex )
                 || !MP2::isOffGameActionObject( world.getTile( visit.tileIndex ).getMainObjectType( false ) ) ) {
                continue;
            }

            const Heroes * hero = world.GetHeroes( visit.heroId );
            if ( hero != nullptr && hero->GetColor() != PlayerColor::NONE ) {
                hero->setVisitedForAllies( visit.tileIndex );
            }
        }

        state.isMarkingAlliedVisits = false;
    }

    // The first human player of the day in the turn order: the lowest color, the human players coming first.
    PlayerColor getFirstHumanInPlay()
    {
        for ( const PlayerColor color : PlayerColorsVector( Multi::getAllHumanColors() ) ) {
            if ( world.GetKingdom( color ).isPlay() ) {
                return color;
            }
        }

        return PlayerColor::NONE;
    }
}

void Multi::onNewDay( const bool isLoadedFromSave )
{
    if ( !isNetworkGame() || isGuest() ) {
        return;
    }

    // A game loaded at the turn of the first human player starts its day there, as a new one.
    dayState().isDayStartPending = !isLoadedFromSave || isFirstTurnOfLoadedGame();

    if ( isLoadedFromSave ) {
        return;
    }

    std::vector<PlayerColorsSet> computerColors;
    PlayerColorsSet allColors = 0;
    for ( const MachineInfo & info : getMachineTable().machines ) {
        computerColors.push_back( info.playedColors );
        allColors |= info.playedColors;
    }

    Measure & state = measure();
    SimultaneousStats & stats = state.stats;

    const uint32_t day = world.CountDay();
    const bool isDayAfterContact = state.isLastDayContact && state.lastDay + 1 == day;

    const ContactVerdict verdict = computeContactVerdict( computerColors );
    if ( verdict.result == ContactResult::NO_PLAYER ) {
        return;
    }

    const bool isConditionMet = hasConditionMet( allColors );

    ++stats.daysMeasured;
    stats.maxMilliseconds = std::max( stats.maxMilliseconds, verdict.milliseconds );

    if ( verdict.alliedOverlaps != 0 ) {
        ++stats.alliedOverlapDays;
    }

    std::string result = contactResultName( verdict.result );

    if ( isConditionMet ) {
        ++stats.conditionDays;
        result = "classic (condition)";
    }
    else if ( verdict.result == ContactResult::SIMULTANEOUS ) {
        ++stats.simultaneousDays;
        if ( !isDayAfterContact ) {
            ++stats.simultaneousDaysWithDelay;
        }
    }
    else if ( verdict.result == ContactResult::ROUND_LIMIT ) {
        ++stats.roundLimitDays;
    }
    else {
        ++stats.contactDays;
    }

    state.lastDay = day;
    state.isLastDayContact = ( verdict.result == ContactResult::CONTACT || verdict.result == ContactResult::ROUND_LIMIT );

    std::ostringstream os;
    os << "[MULTI] Contact day " << day << ": " << result << ", " << verdict.milliseconds << " ms";

    for ( const PlayerReach & reach : verdict.reaches ) {
        os << ", " << Color::String( reach.color ) << " " << reach.stand.count() << "/" << reach.touch.count();
    }

    if ( isTestMode() ) {
        os << describeVerdict( verdict );
    }

    VERBOSE_LOG( os.str() )
}

const Multi::SimultaneousStats & Multi::getSimultaneousStats()
{
    return measure().stats;
}

void Multi::resetSimultaneousStats()
{
    measure() = {};
}

std::string Multi::describeSimultaneousStats()
{
    const SimultaneousStats & stats = measure().stats;

    std::ostringstream os;
    os << "map " << world.w() << "x" << world.h() << ", " << stats.daysMeasured << " day(s) measured, " << stats.simultaneousDays << " simultaneous ("
       << stats.simultaneousDaysWithDelay << " with a day of delay after a contact), " << stats.contactDays << " contact, " << stats.roundLimitDays << " round limit, "
       << stats.conditionDays << " condition, " << stats.alliedOverlapDays << " allied overlap, " << stats.maxMilliseconds << " ms at most";

    return os.str();
}

std::string Multi::describeWorldDifferences( const std::vector<uint8_t> & otherWorld )
{
    WorldSnapshot here;
    WorldSnapshot there;
    if ( otherWorld.empty() || !here.capture().empty() || !there.assign( otherWorld ).empty() ) {
        return {};
    }

    const WorldChanges changes = compareSnapshots( here, there );

    std::ostringstream os;
    os << ":";
    for ( size_t i = 0; i < changes.tiles.size() && i < 6; ++i ) {
        const fheroes2::Point point = Maps::GetPoint( changes.tiles[i] );
        os << " tile [" << point.x << ", " << point.y << "] " << MP2::StringObject( world.getTile( changes.tiles[i] ).getMainObjectType( false ) ) << ";";
    }
    os << " " << changes.tiles.size() << " tile(s), " << changes.fogTiles.size() << " fog only";
    for ( const int32_t id : changes.heroes ) {
        const SaveVersionGuard versionGuard;

        const auto describeHero = []( const WorldSnapshot & snapshot, const int32_t heroId ) {
            const ByteRange & range = snapshot.parts().heroes[heroId];
            const std::vector<uint8_t> bytes( snapshot.data().begin() + static_cast<std::ptrdiff_t>( range.begin ),
                                              snapshot.data().begin() + static_cast<std::ptrdiff_t>( range.end ) );

            ROStreamBuf stream( bytes );
            stream.setBigendian( true );

            Heroes hero;
            stream >> hero;

            std::ostringstream text;
            text << Color::String( hero.GetColor() ) << " on " << hero.GetIndex() << ", " << hero.GetMovePoints() << " move points, recruit "
                 << ( hero.Modes( Heroes::RECRUIT ) ? 1 : 0 ) << ", " << bytes.size() << " bytes";
            return text.str();
        };

        os << "; hero " << id << " here " << describeHero( here, id ) << ", there " << describeHero( there, id );
    }
    for ( const int32_t index : changes.castles ) {
        os << "; castle " << index;
    }
    for ( const int32_t index : changes.kingdoms ) {
        os << "; kingdom " << index << " here / there: " << describeKingdomDifference( here, there, index );
    }
    for ( const int32_t index : changes.capturedObjects ) {
        os << "; captured object " << index;
    }
    for ( const uint32_t uid : changes.mapObjects ) {
        os << "; map object " << uid;
    }
    os << ( changes.isRumorsChanged ? "; rumors" : "" ) << ( changes.isEventsChanged ? "; events" : "" )
       << ( changes.isUltimateArtifactChanged ? "; ultimate artifact" : "" ) << ( changes.isDateChanged ? "; date" : "" ) << ( changes.isSeedChanged ? "; seed" : "" )
       << ( changes.isLayoutChanged ? "; layout" : "" );

    return os.str();
}

void Multi::setSimultaneousTurns( const bool isEnabled )
{
    dayState().isEnabled = isEnabled;
}

void Multi::askClassicDay()
{
    dayState().isClassicDayAsked = true;
}

bool Multi::takeClassicDayRequest()
{
    DayState & state = dayState();

    const bool isAsked = state.isClassicDayAsked;
    state.isClassicDayAsked = false;

    if ( isAsked ) {
        ++state.stats.classicDays;
        VERBOSE_LOG( "[MULTI] Day " << world.CountDay() << " played one after the other: a battle between players was asked" )
    }

    return isAsked;
}

bool Multi::areSimultaneousTurnsEnabled()
{
    return dayState().isEnabled;
}

void Multi::changeSimultaneousTurns( const bool isEnabled )
{
    setSimultaneousTurns( isEnabled );

    if ( !isGuest() ) {
        rememberSimultaneousTurns( isEnabled );
    }

    VERBOSE_LOG( "[MULTI] Simultaneous turns " << ( isEnabled ? "enabled" : "disabled" ) << " here from the next day on" )

    // The host tells the other guests.
    const Message message = makeSimultaneousTurns( { isEnabled, true, getThisMachine() } );
    if ( isGuest() ) {
        sendToMachine( hostMachine, message );
    }
    else {
        sendToOthers( message );
    }
}

void Multi::sendSimultaneousTurns()
{
    sendToOthers( makeSimultaneousTurns( { areSimultaneousTurnsEnabled(), false, hostMachine } ) );
}

void Multi::onSimultaneousTurnsMessage( const Message & message )
{
    const std::optional<SimultaneousTurns> setting = readSimultaneousTurns( message );
    if ( !setting ) {
        ERROR_LOG( "[MULTI] Invalid simultaneous turns setting received" )
        return;
    }

    setSimultaneousTurns( setting->isEnabled );

    if ( !setting->isChangedByPlayer ) {
        return;
    }

    MachineId changedBy = setting->changedBy;

    if ( !isGuest() ) {
        rememberSimultaneousTurns( setting->isEnabled );

        // Every guest learns it, with the computer where it was changed.
        changedBy = message.sender;
        for ( const MachineInfo & info : getMachineTable().machines ) {
            if ( info.id != hostMachine && info.id != message.sender && info.state != MachineState::GONE ) {
                sendToMachine( info.id, makeSimultaneousTurns( { setting->isEnabled, true, changedBy } ) );
            }
        }
    }

    VERBOSE_LOG( "[MULTI] Simultaneous turns " << ( setting->isEnabled ? "enabled" : "disabled" ) << " by computer " << static_cast<int>( changedBy )
                                               << " from the next day on" )

    dayState().notice = setting->isEnabled;
    dayState().noticeFrom = changedBy;
}

void Multi::enableTestToggle()
{
    dayState().isTestToggleEnabled = true;
}

void Multi::toggleForTest()
{
    DayState & state = dayState();
    const uint32_t day = world.CountDay();

    if ( !state.isTestToggleEnabled || !isGuest() || state.lastTestToggleDay == day || ( day != 3 && day != 6 ) ) {
        return;
    }

    state.lastTestToggleDay = day;
    changeSimultaneousTurns( day == 6 );
}

void Multi::showSimultaneousTurnsNotice()
{
    DayState & state = dayState();

    if ( !state.readyNotices.empty() ) {
        const std::vector<MachineId> machines = std::move( state.readyNotices );
        state.readyNotices.clear();

        if ( !isTestMode() && !state.isReadyHere ) {
            for ( const MachineId machine : machines ) {
                std::string text = _( "%{name} has ended their turn." );
                StringReplace( text, "%{name}", getMachineName( machine ) );
                showMapNote( text );
            }
        }
    }

    if ( !state.notice ) {
        return;
    }

    const bool isEnabled = *state.notice;
    state.notice.reset();

    if ( isTestMode() ) {
        return;
    }

    std::string text
        = isEnabled ? _( "%{name} switches to simultaneous turns from tomorrow." ) : _( "%{name} switches back to turns one after the other from tomorrow." );
    StringReplace( text, "%{name}", getMachineName( state.noticeFrom ) );
    showMapNote( text );
}

bool Multi::isSimultaneousDay()
{
    return dayState().isActive && isNetworkGame();
}

void Multi::beginSimultaneousDay()
{
    DayState & state = dayState();
    state.isActive = true;
    state.day = world.CountDay();
    state.playedColors = 0;
    state.isReadyHere = false;
    state.isResumeAsked = false;
    state.isResumeAccepted = false;
    state.isDayDoneSent = false;
    state.isLocked = false;
    state.doneMachines = 0;
    state.closedMachines = 0;
    state.readyNotices.clear();
    state.alliedVisits.clear();
    state.otherAlliedVisits.clear();
    ++state.stats.days;

    // Every computer knows the same table and the same world at the start of the day.
    state.dayMachines.clear();
    for ( const MachineInfo & info : getMachineTable().machines ) {
        if ( info.id == hostMachine || hasPlayerInGame( info.playedColors ) ) {
            state.dayMachines.push_back( info.id );
        }
    }

    reserveCastlesForTheDay();

    VERBOSE_LOG( "[MULTI] Day " << state.day << " played at the same time by " << state.dayMachines.size() << " computer(s)" )
}

void Multi::endSimultaneousDay()
{
    dayState().isActive = false;
}

bool Multi::areOtherDaysDone()
{
    const DayState & state = dayState();
    const MachineSet awaited = getAwaitedGuests();
    return ( awaited & ~state.doneMachines ) == 0;
}

bool Multi::isTurnSkippedToday( const PlayerColor color )
{
    DayState & state = dayState();
    const PlayerColorsSet localColors = getLocalHumanColors();

    // The day starts with its first human player.
    if ( isNetworkGame() && !isGuest() && state.isDayStartPending && ( getAllHumanColors() & color ) != 0 && world.GetKingdom( color ).isPlay() ) {
        const bool isFirstHuman = ( color == getFirstHumanInPlay() );
        state.isDayStartPending = false;

        // Without players here and with one guest left, the game is handed over to it with a classic turn.
        const bool isHandedOver = !hasPlayerInGame( localColors ) && getGuestsWithPlayers().size() <= 1;

        if ( isFirstHuman && !isHandedOver && areSimultaneousTurnsEnabled() && !takeClassicDayRequest() ) {
            startSimultaneousDay();
        }
    }

    if ( !isSimultaneousDay() ) {
        // Who plays a color may change between two turns.
        onTurnBoundary();
        return false;
    }

    if ( isGuest() ) {
        // A vanquished player has nothing to play and the turn is not handed over during simultaneous days.
        return ( localColors & color ) == 0 || !world.GetKingdom( color ).isPlay();
    }

    if ( ( localColors & color ) != 0 ) {
        return false;
    }

    // Played on its computer.
    if ( ( getRemoteHumanColors() & color ) != 0 ) {
        return true;
    }

    // An AI player: every human player must be done first.
    if ( !areOtherDaysDone() ) {
        ++state.stats.waitsForOtherDay;
        waitForOtherDays();
    }

    if ( areOtherDaysDone() ) {
        lockDay();
    }

    return false;
}

bool Multi::isTurnHandedOverAtEnd()
{
    return isGuest() && !isSimultaneousDay();
}

fheroes2::GameMode Multi::onAllPlayersDone( const fheroes2::GameMode mode )
{
    if ( !isSimultaneousDay() || mode != fheroes2::GameMode::END_TURN ) {
        return mode;
    }

    DayState & state = dayState();

    if ( isGuest() ) {
        sendLiveUpdate();

        // DAY_DONE has already been sent if the player ended the turn by hand.
        if ( !state.isDayDoneSent ) {
            sendToOthers( makeDayDone( { state.day, state.isClassicDayAsked, state.alliedVisits } ) );
        }

        // The last changes were sent above. The host can end the day now.
        sendToMachine( hostMachine, makeDayMessage( MessageType::DAY_CLOSED, state.day ) );

        state.isClassicDayAsked = false;

        // Meanwhile the host plays AI players and then ends the day.
        return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
    }

    // Without AI players after the human players, the day is locked here.
    if ( !state.isLocked ) {
        if ( !areOtherDaysDone() ) {
            ++state.stats.waitsForOtherDay;
            waitForOtherDays();
        }

        lockDay();
    }

    // The last changes of the guests must be applied first.
    waitForOtherDaysClosed();

    DayEnd dayEnd;
    dayEnd.day = state.day;
    dayEnd.alliedVisits = state.alliedVisits;
    for ( const auto & [machine, visits] : state.otherAlliedVisits ) {
        dayEnd.alliedVisits.insert( dayEnd.alliedVisits.end(), visits.begin(), visits.end() );
    }

    markAlliedVisits( dayEnd.alliedVisits );

    dayEnd.worldHash = computeWorldHashWithoutRoutes();

    // In self-tests the whole world is sent too, to see what differs on a mismatch.
    if ( isTestMode() ) {
        WorldSnapshot snapshot;
        if ( snapshot.capture().empty() ) {
            dayEnd.world = snapshot.data();
        }
    }

    sendToOthers( makeDayEnd( dayEnd ) );
    endSimultaneousDay();

    // In self-tests the host stops here. It never hands the turn over during simultaneous days.
    if ( isTestDayLimitReached() ) {
        return fheroes2::GameMode::MAIN_MENU;
    }

    return mode;
}

void Multi::checkDayEnd( const DayEnd & dayEnd )
{
    DayState & state = dayState();
    ++state.stats.dayEndsChecked;

    markAlliedVisits( dayEnd.alliedVisits );

    if ( computeWorldHashWithoutRoutes() != dayEnd.worldHash ) {
        ++state.stats.dayEndMismatches;
        ERROR_LOG( "[MULTI] At the end of day " << dayEnd.day << ", the world here differs from the one of the host" << describeWorldDifferences( dayEnd.world ) )
    }
    else {
        VERBOSE_LOG( "[MULTI] End of day " << dayEnd.day << ": same world as the host (live view: " << describeLiveStats() << ", reservations: " << describeReservations()
                                           << ", allied visits: " << dayEnd.alliedVisits.size() << ")" )
    }

    endSimultaneousDay();
}

bool Multi::isAlliedVisitPutOff( const Heroes & hero, const int32_t tileIndex )
{
    DayState & state = dayState();
    if ( !isSimultaneousDay() || state.isMarkingAlliedVisits ) {
        return false;
    }

    // Without allies, only the kingdom of the hero changes.
    const PlayerColor color = hero.GetColor();
    if ( ( Players::GetPlayerFriends( color ) & ~static_cast<PlayerColorsSet>( color ) ) == 0 ) {
        return false;
    }

    state.alliedVisits.push_back( { hero.GetID(), tileIndex } );
    return true;
}

namespace
{
    bool isLastPlayerOfThisComputer( const PlayerColor color )
    {
        const PlayerColorsSet waiting = Multi::getLocalHumanColors() & ~dayState().playedColors & ~static_cast<PlayerColorsSet>( color );

        for ( const PlayerColor other : PlayerColorsVector( waiting ) ) {
            if ( world.GetKingdom( other ).isPlay() ) {
                return false;
            }
        }

        return true;
    }

    bool isConnected()
    {
        const Multi::NetworkSession * session = Multi::getSession();
        return session != nullptr && session->isAlive();
    }
}

bool Multi::confirmEndOfTurn( const PlayerColor color )
{
    DayState & state = dayState();

    if ( !isSimultaneousDay() || !isConnected() || !world.GetKingdom( color ).isPlay() || !isLastPlayerOfThisComputer( color ) ) {
        state.playedColors |= color;
        return true;
    }

    state.isReadyHere = true;
    state.isResumeAsked = false;
    state.isResumeAccepted = false;

    sendLiveUpdate();

    // Every other computer learns it, the host keeps the allied visits of the guests for the evening.
    if ( isGuest() ) {
        sendToOthers( makeDayDone( { state.day, state.isClassicDayAsked, state.alliedVisits } ) );
        state.isDayDoneSent = true;
    }
    else {
        sendToOthers( makeDayDone( { state.day, false, {} } ) );

        if ( areOtherDaysDone() ) {
            lockDay();
            state.playedColors |= color;
            return true;
        }
    }

    VERBOSE_LOG( "[MULTI] Day " << state.day << ": the players here are done, waiting for the other computers" )

    // The actions of the other players are still shown and answered while waiting.
    const auto isWaiting = [&state]() { return isConnected() && !state.isResumeAsked && ( isGuest() ? !state.isLocked : !areOtherDaysDone() ); };

    if ( isWaiting() ) {
        observeUntil( isWaiting );
    }

    if ( !isGuest() && areOtherDaysDone() ) {
        lockDay();
    }

    if ( state.isResumeAsked && !state.isLocked && isConnected() ) {
        sendToOthers( makeDayMessage( MessageType::DAY_RESUME, state.day ) );

        // The host may have ended the day meanwhile. Its answer can be slow (its player in a castle).
        if ( isGuest() ) {
            const auto isAnswerAwaited = [&state]() { return isConnected() && !state.isResumeAccepted && !state.isLocked; };
            if ( isAnswerAwaited() ) {
                observeUntil( isAnswerAwaited );
            }
        }

        if ( !isGuest() || state.isResumeAccepted ) {
            VERBOSE_LOG( "[MULTI] Day " << state.day << ": the player here takes their day back" )

            state.isReadyHere = false;
            state.isDayDoneSent = false;
            refreshMap();
            return false;
        }
    }

    state.playedColors |= color;
    return true;
}

bool Multi::isDayLocked()
{
    return dayState().isLocked;
}

void Multi::restartOtherDay( const MachineId machine )
{
    DayState & state = dayState();
    state.doneMachines &= static_cast<MachineSet>( ~machineBit( machine ) );
    state.closedMachines &= static_cast<MachineSet>( ~machineBit( machine ) );
    state.otherAlliedVisits.erase( machine );
}

bool Multi::canResumeDay()
{
    const DayState & state = dayState();
    return isSimultaneousDay() && state.isReadyHere && !state.isLocked && !state.isResumeAsked;
}

void Multi::askDayResume()
{
    dayState().isResumeAsked = true;
}

void Multi::offerDayResume()
{
    if ( canResumeDay() && Dialog::YES == fheroes2::showStandardTextMessage( _( "End Turn" ), _( "Do you want to take your turn back?" ), Dialog::YES | Dialog::NO ) ) {
        askDayResume();
    }
}

std::string Multi::getObserverBannerText()
{
    if ( !isSpectatedBattleAvailable() ) {
        return {};
    }

    return System::isHandheldDevice() ? _( "Battle of your ally (touch here to watch)" ) : _( "Battle of your ally (B to watch)" );
}

void Multi::onDayReadinessMessage( const Message & message )
{
    DayState & state = dayState();
    const MachineSet sender = machineBit( message.sender );

    switch ( message.type ) {
    case MessageType::DAY_DONE: {
        const std::optional<DayDone> dayDone = readDayDone( message );
        if ( !dayDone || dayDone->day != state.day ) {
            break;
        }

        state.doneMachines |= sender;
        if ( !state.isReadyHere ) {
            state.readyNotices.push_back( message.sender );
        }

        if ( !isGuest() ) {
            state.otherAlliedVisits[message.sender] = dayDone->alliedVisits;
            if ( dayDone->isBattleAsked ) {
                state.isClassicDayAsked = true;
            }
            VERBOSE_LOG( "[MULTI] Computer " << static_cast<int>( message.sender ) << " played its day" )
        }
        break;
    }
    case MessageType::DAY_RESUME:
        if ( isGuest() || !state.isLocked ) {
            state.doneMachines &= static_cast<MachineSet>( ~sender );
        }

        if ( !isGuest() && !state.isLocked ) {
            sendToMachine( message.sender, makeDayMessage( MessageType::DAY_RESUMED, state.day ) );
            VERBOSE_LOG( "[MULTI] Day " << state.day << ": computer " << static_cast<int>( message.sender ) << " takes its day back" )
        }
        break;
    case MessageType::DAY_RESUMED:
        state.isResumeAccepted = true;
        break;
    case MessageType::DAY_LOCKED:
        state.isLocked = true;
        break;
    case MessageType::DAY_CLOSED:
        if ( !isGuest() ) {
            state.closedMachines |= sender;
        }
        break;
    default:
        break;
    }
}

bool Multi::areOtherDaysClosed()
{
    const MachineSet awaited = getAwaitedGuests();
    return ( awaited & ~dayState().closedMachines ) == 0;
}

void Multi::setOtherDayClosed( const MachineId machine )
{
    dayState().closedMachines |= machineBit( machine );
}

void Multi::processDayReadinessMessages()
{
    NetworkSession * session = getSession();
    if ( session == nullptr ) {
        return;
    }

    // Read in order. A day resumed then ended is not the same as ended then resumed.
    while ( std::optional<Message> message
            = session->receive( { MessageType::DAY_DONE, MessageType::DAY_RESUME, MessageType::DAY_RESUMED, MessageType::DAY_LOCKED, MessageType::DAY_CLOSED } ) ) {
        onDayReadinessMessage( *message );
    }
}

bool Multi::isHeroForHireHere( const int heroId )
{
    const DayState & state = dayState();
    if ( !isSimultaneousDay() || state.dayMachines.size() < 2 ) {
        return true;
    }

    // The share changes every day and is the same on every computer.
    const uint32_t mix = ( static_cast<uint32_t>( heroId ) * 2654435761U ) ^ ( world.CountDay() * 40503U );
    const size_t share = ( mix >> 7 ) % state.dayMachines.size();

    return state.dayMachines[share] == getThisMachine();
}

bool Multi::canCastAdventureSpell( const Heroes & hero, const int spellId )
{
    if ( !isSimultaneousDay() || hero.isControlHuman() ) {
        return true;
    }

    switch ( spellId ) {
    case Spell::DIMENSIONDOOR:
    case Spell::TOWNGATE:
    case Spell::TOWNPORTAL:
    case Spell::SUMMONBOAT:
    case Spell::HAUNT:
    case Spell::SETEGUARDIAN:
    case Spell::SETAGUARDIAN:
    case Spell::SETFGUARDIAN:
    case Spell::SETWGUARDIAN:
        return false;
    default:
        break;
    }

    return true;
}

const Multi::SimultaneousDayStats & Multi::getSimultaneousDayStats()
{
    return dayState().stats;
}

std::string Multi::describeSimultaneousDays()
{
    const SimultaneousDayStats & stats = dayState().stats;

    std::ostringstream os;
    os << stats.days << " day(s) played at the same time, " << stats.classicDays << " played one after the other for a battle, " << stats.waitsForOtherDay
       << " wait(s) for the other computers, " << stats.dayEndsChecked << " day end(s) checked, " << stats.dayEndMismatches << " mismatch(es)";

    return os.str();
}
