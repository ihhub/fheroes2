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

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Multi
{
    // Increase this value whenever the content of any message changes.
    constexpr uint32_t protocolVersion{ 20 };

    // Computers of a game: the host is 0, the host gives each guest a number from 1 to 5. Up to five guests play at once.
    using MachineId = uint8_t;
    constexpr MachineId hostMachine{ 0 };
    constexpr MachineId maxMachineId{ 5 };
    constexpr size_t maxGuests{ 5 };

    // One bit per computer.
    using MachineSet = uint8_t;

    constexpr MachineSet machineBit( const MachineId machine )
    {
        return static_cast<MachineSet>( 1U << machine );
    }

    // The six colors of PlayerColor.
    constexpr uint8_t allPlayerColors{ 0x3F };

    // Big-endian like save files: "fh2" marker (3 bytes), message type (4 bytes), payload size (4 bytes).
    constexpr size_t messageHeaderSize{ 11 };

    // A bigger payload is treated as corrupted data. A compressed save file takes a few hundred KB.
    constexpr uint32_t maxMessagePayloadSize{ 32 * 1024 * 1024 };

    enum class MessageType : uint32_t
    {
        HANDSHAKE = 0,
        HANDSHAKE_REPLY = 1,
        TURN_STATE = 2,
        GAME_END = 3,
        // Same as TURN_STATE but only to show the game. The color is a player of the receiving computer.
        GAME_SNAPSHOT = 4,
        BATTLE_START = 5,
        BATTLE_ACTIONS = 6,
        BATTLE_END = 7,
        HERO_LEVEL_UP = 8,
        HERO_LEVEL_UP_CHOICE = 9,
        HERO_NOTICE = 10,
        GAME_RESULT = 11,
        // World units changed by the last action of the sender, shown live on the other computer.
        WORLD_UPDATE = 12,
        // Simultaneous turns. The guest reserves on the host what an action is going to change.
        RESERVE = 13,
        RESERVE_REPLY = 14,
        // Same as TURN_STATE. Starts a day played by both computers at the same time.
        DAY_START = 15,
        // The guest has played the day for all its players.
        DAY_DONE = 16,
        // Hash of the host world, which must match the guest one.
        DAY_END = 17,
        // Simultaneous turns from the next day or not. Sent by the host at start and by either computer when its player changes it.
        SIMULTANEOUS_TURNS = 18,
        // Simultaneous turns. A hero attacks a hero or a castle of the other computer, whose player has to be stopped.
        // LIVE_BATTLE_DONE comes after the changes made by the battle, or when the attack is given up.
        BATTLE_REQUEST = 19,
        BATTLE_REQUEST_REPLY = 20,
        LIVE_BATTLE_DONE = 21,
        // Simultaneous turns. A computer takes its ended day back, the host agrees with DAY_RESUMED.
        // DAY_LOCKED ends the day once both are done. The host sends DAY_DONE too, to tell the guest.
        DAY_RESUME = 22,
        DAY_RESUMED = 23,
        DAY_LOCKED = 24,
        // Battle watched by an ally. Same as BATTLE_START (without the game), BATTLE_ACTIONS and BATTLE_END. Nothing is sent back.
        SPECTATE_START = 25,
        SPECTATE_ACTIONS = 26,
        SPECTATE_END = 27,
        // Session messages, never seen by the game (see NetworkSession).
        SESSION_ID = 28,
        SESSION_ACK = 29,
        SESSION_RESUME = 30,
        SESSION_RESUME_REPLY = 31,
        SESSION_CLOSE = 32,
        // Simultaneous turns. Sent by the guest after its last changes of a locked day. The host waits for it before DAY_END.
        DAY_CLOSED = 33,
        // The waiting room before the game starts (see multi_lobby_protocol.h). The host sends GAME_START when its player
        // starts the game and waits for the same message back: chat lines sent before it never reach the game.
        WAITING_ROOM = 34,
        PLAYER_INFO = 35,
        CHAT = 36,
        GAME_START = 37,
        // A message between guests, through the host. Never seen by the game (see NetworkSession).
        ROUTED = 38,
        // The computers of the game and their players, sent by the host at the start and whenever it changes.
        MACHINES = 39,

        // Used only by self-tests.
        TEST_DATA = 1000,
        TEST_DATA_REPLY = 1001,

        // Between a game and a lobby server (see multi_lobby_protocol.h).
        LOBBY_HELLO = 2000,
        LOBBY_WELCOME = 2001,
        LOBBY_STATS = 2002,
        LOBBY_KEEPALIVE = 2003,
        ROOM_PUBLISH = 2010,
        ROOM_STATUS = 2011,
        ROOM_UPDATE = 2012,
        ROOM_CLOSE = 2013,
        ROOM_LIST_SUBSCRIBE = 2020,
        ROOM_LIST = 2021,
        ROOM_INFO = 2022,
        ROOM_REMOVED = 2023,
        ROOM_JOIN = 2030,
        JOIN_PLAN = 2031,
        RELAY_HOST = 2040,
        RELAY_GUEST = 2041,
        RELAY_READY = 2042,
        RELAY_REFUSED = 2043,
        // Sent by the lobby to the game port of a host, to know if it can be reached directly.
        LOBBY_PROBE = 2050,
        LOBBY_PROBE_REPLY = 2051
    };

    struct Message
    {
        MessageType type{ MessageType::HANDSHAKE };
        std::vector<uint8_t> payload;
        // Arrival order and the computer which sent it, set by the receiving session.
        uint64_t arrival{ 0 };
        MachineId sender{ hostMachine };
    };

    // Returns the header followed by the payload.
    std::vector<uint8_t> encodeMessage( const Message & message );

    // Returns false for a wrong marker or a payload that is too big.
    bool decodeMessageHeader( const uint8_t * header, MessageType & type, uint32_t & payloadSize );

    // A whole game, a battle on the current world or the end of a day. What comes after depends on it and is never taken before it.
    bool isOrderBarrier( const MessageType type );

    bool isSessionMessage( const MessageType type );

    // Sent by the guest to take a lost connection back.
    struct SessionResume
    {
        uint64_t sessionId{ 0 };
        uint64_t receivedCount{ 0 };
    };

    struct SessionResumeReply
    {
        bool isAccepted{ false };
        uint64_t receivedCount{ 0 };
    };

    Message makeSessionId( const uint64_t sessionId );
    std::optional<uint64_t> readSessionId( const Message & message );

    Message makeSessionAck( const uint64_t receivedCount );
    std::optional<uint64_t> readSessionAck( const Message & message );

    Message makeSessionResume( const SessionResume & resume );
    std::optional<SessionResume> readSessionResume( const Message & message );

    Message makeSessionResumeReply( const SessionResumeReply & reply );
    std::optional<SessionResumeReply> readSessionResumeReply( const Message & message );

    // A message of a guest for other computers, carried by the host. From a guest the sender is not read, the host knows it.
    struct RoutedMessage
    {
        MachineId sender{ hostMachine };
        MachineSet targets{ 0 };
        Message inner;
    };

    Message makeRouted( const RoutedMessage & routed );
    // Returns nothing for a session message or a routed message inside.
    std::optional<RoutedMessage> readRouted( const Message & message );

    // The host answers HANDSHAKE with HANDSHAKE_REPLY and closes the connection if refused. Both messages start as in the
    // network design of the project (the game version, then the result and the public key of the host), our own fields come
    // after.
    struct HandshakeInfo
    {
        // Major, minor and intermediate version.
        std::array<uint8_t, 3> gameVersion{};
        uint32_t protocolVersion{ 0 };
        // Identifies the sources the game was built from (see getBuildId()).
        uint64_t buildId{ 0 };
        // Not checked, the same sources play together on any system.
        std::string platform;
        // Random, kept on the computer: a guest which comes back after a restart is recognised by it.
        uint64_t machineKey{ 0 };
        // UTF-8, may be empty.
        std::string nickname;
        // The colors this computer played, when it joins again a game whose host was restarted.
        uint8_t claimedColors{ 0 };
    };

    // The codes of the network design, then ours.
    enum class HandshakeResult : uint8_t
    {
        ACCEPTED = 0,
        INVALID_MESSAGE = 1,
        INCOMPATIBLE_VERSIONS = 2,
        NOT_A_HOST = 3,
        // The same version built from other sources.
        BUILD_MISMATCH = 4,
        // Every human color is taken.
        GAME_FULL = 5,
        // The game goes on without this computer, it never played in it.
        GAME_STARTED = 6
    };

    // Public key of the host, all zeros as long as messages are not encrypted.
    constexpr size_t handshakeKeySize{ 16 };

    struct HandshakeReply
    {
        HandshakeResult result{ HandshakeResult::INVALID_MESSAGE };
        std::array<uint8_t, handshakeKeySize> publicKey{};
        HandshakeInfo host;
    };

    HandshakeInfo getLocalHandshakeInfo();

    HandshakeResult checkHandshake( const HandshakeInfo & local, const HandshakeInfo & remote );

    std::string describeHandshakeResult( const HandshakeResult result, const HandshakeInfo & local, const HandshakeInfo & remote );

    // Errors are logged in English as "reason: detail: detail". Translates each part which has a translation.
    std::string translateError( const std::string & error );

    Message makeHandshake( const HandshakeInfo & info );
    std::optional<HandshakeInfo> readHandshake( const Message & message );

    Message makeHandshakeReply( const HandshakeResult result, const HandshakeInfo & host );
    std::optional<HandshakeReply> readHandshakeReply( const Message & message );

    // Stage of the turn of the player stored in a save file.
    enum class TurnStage : uint8_t
    {
        // A regular save file. The player continues the turn.
        IN_PROGRESS = 0,
        // Income, events and the new week are still to be processed.
        NOT_STARTED = 1,
        // The game continues with the next player.
        ENDED = 2
    };

    // The whole game, sent whenever a turn is handed over.
    struct TurnState
    {
        uint8_t color{ 0 };
        // Human players of the receiving computer.
        uint8_t receiverColors{ 0 };
        TurnStage stage{ TurnStage::NOT_STARTED };
        // No player left on the sending computer, the receiving one continues alone.
        bool isSenderOut{ false };
        // Fingerprint of the game state once saved, checked after loading on the other side.
        uint64_t fingerprint{ 0 };
        // Content of a save file.
        std::vector<uint8_t> saveData;
    };

    // The type can be TURN_STATE or GAME_SNAPSHOT.
    Message makeTurnState( const TurnState & state, const MessageType type = MessageType::TURN_STATE );
    std::optional<TurnState> readTurnState( const Message & message );

    // Played on both computers from the same start. The battle ID tells apart the messages of a battle given up.
    struct BattleStart
    {
        enum class Defender : uint8_t
        {
            HERO = 0,
            CASTLE = 1,
            // Monsters on the tile of the battle (only for watched battles).
            MONSTERS = 2
        };

        uint32_t battleId{ 0 };
        uint32_t attackerHeroId{ 0 };
        Defender defender{ Defender::HERO };
        // Not used for a castle, the tile index tells which one it is.
        uint32_t defenderHeroId{ 0 };
        int32_t tileIndex{ -1 };
        uint32_t seed{ 0 };
        // Human players of the receiving computer.
        uint8_t followerColors{ 0 };
        uint64_t fingerprint{ 0 };
        // Shown on the leading computer. Castle walls and the direction units face depend on it.
        bool isShown{ true };
        // For monsters, the type of the object on the tile they are taken from (MP2::MapObjectType).
        uint16_t defenderObject{ 0 };
        // The game just before the battle. Empty during a simultaneous day, the fingerprint then covers only both armies.
        std::vector<uint8_t> saveData;
    };

    Message makeBattleStart( const BattleStart & start );
    std::optional<BattleStart> readBattleStart( const Message & message );

    // Converts a battle message into the matching message of a watched battle.
    Message makeSpectatedMessage( Message message );
    // Reverse of the above. Returns nothing for other messages.
    std::optional<Message> readSpectatedMessage( const Message & message );

    // Orders given for one unit, as stored in battle commands.
    struct BattleCommandData
    {
        uint32_t type{ 0 };
        std::vector<int32_t> values;
    };

    struct BattleActions
    {
        uint32_t battleId{ 0 };
        std::vector<BattleCommandData> commands;
    };

    Message makeBattleActions( const BattleActions & actions );
    std::optional<BattleActions> readBattleActions( const Message & message );

    struct BattleEnd
    {
        uint32_t battleId{ 0 };
        // Fingerprint of the battle result, or 0 if the battle was given up.
        uint64_t fingerprint{ 0 };
    };

    Message makeBattleEnd( const BattleEnd & end );
    std::optional<BattleEnd> readBattleEnd( const Message & message );

    // A hero of the receiving computer gains a level, its player chooses the skill.
    struct HeroLevelUp
    {
        uint32_t heroId{ 0 };
        int32_t primarySkill{ 0 };
        // Offered secondary skills with the level they would reach, or Skill::Secondary::UNKNOWN if a skill is not offered.
        int32_t firstSkill{ 0 };
        int32_t firstSkillLevel{ 0 };
        int32_t secondSkill{ 0 };
        int32_t secondSkillLevel{ 0 };
    };

    Message makeHeroLevelUp( const HeroLevelUp & levelUp );
    std::optional<HeroLevelUp> readHeroLevelUp( const Message & message );

    // The chosen secondary skill, or Skill::Secondary::UNKNOWN if none was chosen.
    Message makeHeroLevelUpChoice( const int32_t skill );
    std::optional<int32_t> readHeroLevelUpChoice( const Message & message );

    // Shown to the player of the hero, on their computer.
    struct HeroNotice
    {
        enum class Kind : uint8_t
        {
            // Number of skeletons.
            RAISED_SKELETONS = 0,
            // IDs of the spells learned with Eagle Eye.
            LEARNED_SPELLS = 1,
            // ID of the assembled artifact. The text (not translated) describes how it was assembled.
            ASSEMBLED_ARTIFACT = 2
        };

        Kind kind{ Kind::RAISED_SKELETONS };
        uint32_t heroId{ 0 };
        std::vector<int32_t> values;
        std::string text;
    };

    Message makeHeroNotice( const HeroNotice & notice );
    std::optional<HeroNotice> readHeroNotice( const Message & message );

    // A player won or lost. The result is a GameOver condition.
    struct GameResult
    {
        uint8_t color{ 0 };
        uint32_t result{ 0 };
        // The game is over for everyone.
        bool isGameOver{ false };
        // No player left on the receiving computer, the other one continues alone.
        bool isOverForReceiver{ false };
    };

    Message makeGameResult( const GameResult & gameResult );
    std::optional<GameResult> readGameResult( const Message & message );

    struct WorldUnit
    {
        enum class Kind : uint8_t
        {
            TILE = 1,
            HERO = 2,
            CASTLE = 3,
            KINGDOM = 4,
            CAPTURED_OBJECT = 5,
            // An event, a sphinx or a sign, by UID. Empty data means the object was removed.
            MAP_OBJECT = 6,
            // Fog of a tile only, one byte with the colors still under fog.
            FOG = 7
        };

        Kind kind{ Kind::TILE };

        // Tile index, hero id, tile index of the castle center, kingdom index or tile index of the captured object.
        int32_t key{ -1 };

        // The unit as written in save files.
        std::vector<uint8_t> data;
    };

    struct WorldUpdate
    {
        uint32_t sequence{ 0 };

        // Hash of the world of the sender after applying the units.
        uint64_t worldHash{ 0 };

        // Something changed which cannot be sent as units. The receiver waits for the next full game.
        bool isIncomplete{ false };

        std::vector<WorldUnit> units;
    };

    Message makeWorldUpdate( const WorldUpdate & update );
    std::optional<WorldUpdate> readWorldUpdate( const Message & message );

    struct ReserveRequest
    {
        uint32_t requestId{ 0 };
        // Reservations last until the end of the day.
        uint32_t day{ 0 };
        uint8_t kingdoms{ 0 };
        std::vector<int32_t> tiles;
    };

    Message makeReserveRequest( const ReserveRequest & request );
    std::optional<ReserveRequest> readReserveRequest( const Message & message );

    struct ReserveReply
    {
        uint32_t requestId{ 0 };
        bool isGranted{ false };
        // When refused, the first tile already reserved by the host, or -1 for a kingdom.
        int32_t takenTile{ -1 };
    };

    Message makeReserveReply( const ReserveReply & reply );
    std::optional<ReserveReply> readReserveReply( const Message & message );

    // Marked as visited for the allies of the hero at the end of a simultaneous day. An ally may be played on the other computer.
    struct AlliedVisit
    {
        int32_t heroId{ 0 };
        int32_t tileIndex{ 0 };
    };

    struct DayEnd
    {
        uint32_t day{ 0 };
        uint64_t worldHash{ 0 };

        // Visits of both computers, marked in this order before comparing the worlds.
        std::vector<AlliedVisit> alliedVisits;

        // Used only by self-tests. The whole world of the host to find what differs.
        std::vector<uint8_t> world;
    };

    struct DayDone
    {
        uint32_t day{ 0 };
        // A player tried to fight the other computer. The next day is played one after another.
        bool isBattleAsked{ false };

        std::vector<AlliedVisit> alliedVisits;
    };

    Message makeDayDone( const DayDone & dayDone );
    std::optional<DayDone> readDayDone( const Message & message );

    Message makeDayEnd( const DayEnd & dayEnd );
    std::optional<DayEnd> readDayEnd( const Message & message );

    struct BattleRequest
    {
        uint32_t requestId{ 0 };
        uint32_t attackerHeroId{ 0 };
        int32_t targetIndex{ -1 };
    };

    Message makeBattleRequest( const BattleRequest & request );
    std::optional<BattleRequest> readBattleRequest( const Message & message );

    struct BattleRequestReply
    {
        uint32_t requestId{ 0 };
        bool isAccepted{ false };
    };

    Message makeBattleRequestReply( const BattleRequestReply & reply );
    std::optional<BattleRequestReply> readBattleRequestReply( const Message & message );

    Message makeLiveBattleDone( const uint32_t requestId );
    std::optional<uint32_t> readLiveBattleDone( const Message & message );

    // DAY_RESUME, DAY_RESUMED and DAY_LOCKED messages contain only the day.
    Message makeDayMessage( const MessageType type, const uint32_t day );
    std::optional<uint32_t> readDayMessage( const Message & message, const MessageType type );

    struct SimultaneousTurns
    {
        bool isEnabled{ false };
        // Changed by a player, the local player is told.
        bool isChangedByPlayer{ false };
        // The computer of that player.
        MachineId changedBy{ hostMachine };
    };

    Message makeSimultaneousTurns( const SimultaneousTurns & setting );
    std::optional<SimultaneousTurns> readSimultaneousTurns( const Message & message );
}
