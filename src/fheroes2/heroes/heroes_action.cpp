/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "ai_hero_action.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "game_static.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "mus.h"
#include "payment.h"
#include "players.h"
#include "profit.h"
#include "puzzle.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_monster.h"
#include "ui_text.h"
#include "visit.h"
#include "world.h"

namespace
{
    class MusicalEffectPlayer
    {
    public:
        MusicalEffectPlayer() = default;

        explicit MusicalEffectPlayer( const int trackId )
        {
            play( trackId );
        }

        MusicalEffectPlayer( const MusicalEffectPlayer & ) = delete;

        ~MusicalEffectPlayer()
        {
            Music::setVolume( 100 * Settings::Get().MusicVolume() / 10 );
        }

        MusicalEffectPlayer & operator=( const MusicalEffectPlayer & ) = delete;

        static bool canBePlayed( const int trackId )
        {
            if ( trackId == MUS::UNUSED || trackId == MUS::UNKNOWN ) {
                return false;
            }

            // There are no "musical" effects in MIDI format
            if ( Settings::Get().MusicMIDI() ) {
                return false;
            }

            return AudioManager::isExternalMusicFileAvailable( trackId );
        }

        static void play( const int trackId )
        {
            if ( !canBePlayed( trackId ) ) {
                return;
            }

            // "Musical" sound effects should use the volume of the sound effects instead of the volume of the music
            const int soundVolume = Settings::Get().SoundVolume();

            // Play the sound effect only if the volume of the sound effect is not set to 0
            if ( soundVolume <= 0 ) {
                return;
            }

            Music::setVolume( 100 * soundVolume / 10 );
            AudioManager::PlayMusic( trackId, Music::PlaybackMode::PLAY_ONCE );
        }

    private:
        const AudioManager::MusicRestorer _musicRestorer;
    };

    void DialogCaptureResourceObject( std::string hdr, std::string msg, const int32_t resourceType )
    {
        const Funds info = ProfitConditions::FromMine( resourceType );
        int32_t resourceCount = 0;

        switch ( resourceType ) {
        case Resource::MERCURY:
            resourceCount = info.mercury;
            break;
        case Resource::WOOD:
            resourceCount = info.wood;
            break;
        case Resource::ORE:
            resourceCount = info.ore;
            break;
        case Resource::SULFUR:
            resourceCount = info.sulfur;
            break;
        case Resource::CRYSTAL:
            resourceCount = info.crystal;
            break;
        case Resource::GEMS:
            resourceCount = info.gems;
            break;
        case Resource::GOLD:
            resourceCount = info.gold;
            break;
        default:
            // You're passing an invalid resource type. Check your logic!
            assert( 0 );
            break;
        }

        std::string perday = _( "%{count} / day" );
        StringReplace( perday, "%{count}", resourceCount );

        switch ( resourceCount ) {
        case 1:
            StringReplace( msg, "%{count}", _( "one" ) );
            break;
        case 2:
            StringReplace( msg, "%{count}", _( "two" ) );
            break;
        default:
            StringReplace( msg, "%{count}", resourceCount );
            break;
        }

        fheroes2::ResourceDialogElement resourceUI( resourceType, std::move( perday ) );

        fheroes2::showStandardTextMessage( std::move( hdr ), std::move( msg ), Dialog::OK, { &resourceUI } );
    }

    void BattleLose( Heroes & hero, const Battle::Result & res, bool attacker )
    {
        const uint32_t reason = attacker ? res.AttackerResult() : res.DefenderResult();

        AudioManager::PlaySound( M82::KILLFADE );

        hero.FadeOut();
        hero.Dismiss( reason );

        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        if ( !hero.GetKingdom().isLoss() ) {
            // If the enemy is not vanquished we update only position of defeated hero on radar to remove hero mark.
            const fheroes2::Point heroPosition = hero.GetCenter();
            I.getRadar().SetRenderArea( { heroPosition.x, heroPosition.y, 1, 1 } );
        }
        // If the enemy is vanquished we do not set radar ROI and fully redraw the radar map image as there might be color reset of enemy's objects.

        I.setRedraw( Interface::REDRAW_RADAR );
    }

    void runActionObjectFadeOutAnumation( const Maps::Tiles & tile, const MP2::MapObjectType objectType )
    {
        uint32_t objectUID = 0;

        if ( Maps::getObjectTypeByIcn( tile.getObjectIcnType(), tile.GetObjectSpriteIndex() ) == objectType ) {
            objectUID = tile.GetObjectUID();
        }
        else {
            // In maps made by the original map editor the action object can be in the bottom layer addons.
            for ( auto iter = tile.getBottomLayerAddons().rbegin(); iter != tile.getBottomLayerAddons().rend(); ++iter ) {
                if ( Maps::getObjectTypeByIcn( iter->_objectIcnType, iter->_imageIndex ) == objectType ) {
                    objectUID = iter->_uid;
                    break;
                }
            }
        }

        assert( objectUID != 0 );

        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        I.getGameArea().runSingleObjectAnimation( std::make_shared<Interface::ObjectFadingOutInfo>( objectUID, tile.GetIndex(), objectType ) );

        // Update radar in the place of the removed object.
        I.getRadar().SetRenderArea( { Maps::GetPoint( tile.GetIndex() ), { 1, 1 } } );
        I.setRedraw( Interface::REDRAW_RADAR );
    }

    void RecruitMonsterFromTile( Heroes & hero, Maps::Tiles & tile, std::string msg, const Troop & troop, const bool remove )
    {
        if ( !hero.GetArmy().CanJoinTroop( troop ) )
            fheroes2::showStandardTextMessage( std::move( msg ), _( "You are unable to recruit at this time, your ranks are full." ), Dialog::OK );
        else {
            const uint32_t recruit = Dialog::RecruitMonster( troop.GetMonster(), troop.GetCount(), false, 0 ).GetCount();

            if ( recruit ) {
                if ( remove && recruit == troop.GetCount() ) {
                    Game::PlayPickupSound();

                    runActionObjectFadeOutAnumation( tile, tile.GetObject() );

                    resetObjectMetadata( tile );
                }
                else {
                    setMonsterCountOnTile( tile, troop.GetCount() - recruit );
                }

                const Funds paymentCosts = troop.GetMonster().GetCost() * recruit;
                hero.GetKingdom().OddFundsResource( paymentCosts );

                hero.GetArmy().JoinTroop( troop.GetMonster(), recruit, false );

                Interface::AdventureMap::Get().setRedraw( Interface::REDRAW_STATUS );
            }
        }
    }

    void WhirlpoolTroopLoseEffect( Heroes & hero )
    {
        const Army & heroArmy = hero.GetArmy();

        Troop * weakestTroop = heroArmy.GetWeakestTroop();
        assert( weakestTroop != nullptr );
        if ( weakestTroop == nullptr ) {
            return;
        }

        // Whirlpool effect affects heroes only with more than one creature in more than one slot
        if ( heroArmy.GetOccupiedSlotCount() == 1 && weakestTroop->GetCount() == 1 ) {
            return;
        }

        if ( 1 == Rand::Get( 1, 3 ) ) {
            fheroes2::showStandardTextMessage( MP2::StringObject( MP2::MapObjectType::OBJ_WHIRLPOOL ),
                                               _( "A whirlpool engulfs your ship. Some of your army has fallen overboard." ), Dialog::OK );

            if ( weakestTroop->GetCount() == 1 ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " lost " << weakestTroop->GetCount() << " " << weakestTroop->GetName() << " in the whirlpool" )

                weakestTroop->Reset();
            }
            else {
                const uint32_t newCount = Monster::GetCountFromHitPoints( weakestTroop->GetID(), weakestTroop->GetHitPoints()
                                                                                                     - weakestTroop->GetHitPoints() * Game::GetWhirlpoolPercent() / 100 );

                DEBUG_LOG( DBG_GAME, DBG_INFO,
                           hero.GetName() << " lost " << weakestTroop->GetCount() - newCount << " " << weakestTroop->GetName() << " in the whirlpool" )

                weakestTroop->SetCount( newCount );
            }

            Interface::AdventureMap::Get().setRedraw( Interface::REDRAW_STATUS );
        }
    }

    void ActionToMonster( Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Troop troop = getTroopFromTile( tile );

        Interface::AdventureMap & I = Interface::AdventureMap::Get();

        bool destroy = false;

        const NeutralMonsterJoiningCondition join = Army::GetJoinSolution( hero, tile, troop );

        if ( join.reason == NeutralMonsterJoiningCondition::Reason::Alliance ) {
            if ( hero.GetArmy().CanJoinTroop( troop ) ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, troop.GetName() << " join " << hero.GetName() << " as a part of alliance" )

                assert( join.joiningMessage != nullptr );
                fheroes2::showStandardTextMessage( "", join.joiningMessage, Dialog::OK );
                hero.GetArmy().JoinTroop( troop );
            }
            else {
                DEBUG_LOG( DBG_GAME, DBG_INFO, troop.GetName() << " unblock way for " << hero.GetName() << " as a part of alliance" )

                assert( join.fleeingMessage != nullptr );
                fheroes2::showStandardTextMessage( "", join.fleeingMessage, Dialog::OK );
            }

            destroy = true;
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::Bane ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, troop.GetName() << " run away from " << hero.GetName() << " as a part of bane" )

            assert( join.fleeingMessage != nullptr );
            fheroes2::showStandardTextMessage( "", join.fleeingMessage, Dialog::OK );
            destroy = true;
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::Free ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, troop.GetName() << " want to join " << hero.GetName() )

            // This condition must already be met if a group of monsters wants to join
            assert( hero.GetArmy().CanJoinTroop( troop ) );

            if ( Dialog::YES == Dialog::ArmyJoinFree( troop ) ) {
                hero.GetArmy().JoinTroop( troop );

                I.setRedraw( Interface::REDRAW_STATUS );
                destroy = true;
            }
            else {
                fheroes2::showStandardTextMessage( "", _( "Insulted by your refusal of their offer, the monsters attack!" ), Dialog::OK );
            }
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::ForMoney ) {
            const int32_t joiningCost = troop.GetTotalCost().gold;

            DEBUG_LOG( DBG_GAME, DBG_INFO, join.monsterCount << " " << troop.GetName() << " want to join " << hero.GetName() << " for " << joiningCost << " gold" )

            // These conditions must already be met if a group of monsters wants to join
            assert( hero.GetArmy().CanJoinTroop( troop ) && hero.GetKingdom().AllowPayment( Funds( Resource::GOLD, joiningCost ) ) );

            if ( Dialog::YES == Dialog::ArmyJoinWithCost( troop, join.monsterCount, joiningCost ) ) {
                hero.GetArmy().JoinTroop( troop.GetMonster(), join.monsterCount, false );
                hero.GetKingdom().OddFundsResource( Funds( Resource::GOLD, joiningCost ) );

                I.setRedraw( Interface::REDRAW_STATUS );
                destroy = true;
            }
            else {
                fheroes2::showStandardTextMessage( "", _( "Insulted by your refusal of their offer, the monsters attack!" ), Dialog::OK );
            }
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::RunAway ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, troop.GetName() << " run away from " << hero.GetName() )

            std::string message = _( "The %{monster}, awed by the power of your forces, begin to scatter.\nDo you wish to pursue and engage them?" );
            StringReplaceWithLowercase( message, "%{monster}", troop.GetMultiName() );

            if ( fheroes2::showStandardTextMessage( "", std::move( message ), Dialog::YES | Dialog::NO ) == Dialog::NO ) {
                destroy = true;
            }
        }

        // Fight
        if ( !destroy ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " attacks monster " << troop.GetName() )

            // Set the hero's attacked monster tile index and immediately redraw game area to show an attacking sprite for this monster
            hero.SetAttackedMonsterTileIndex( dst_index );

            I.redraw( Interface::REDRAW_GAMEAREA );

            Army army( tile );

            const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );

            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );

                destroy = true;
            }
            else {
                BattleLose( hero, res, true );

                // The army can include both the original monsters and their upgraded version
                const uint32_t monstersLeft = army.getTotalCount();
#ifndef NDEBUG
                const Monster originalMonster = troop.GetMonster();
                const Monster upgradedMonster = originalMonster.GetUpgrade();

                if ( upgradedMonster == originalMonster ) {
                    assert( monstersLeft == army.GetCountMonsters( originalMonster ) );
                }
                else {
                    assert( monstersLeft == army.GetCountMonsters( originalMonster ) + army.GetCountMonsters( upgradedMonster ) );
                }
#endif

                if ( monstersLeft > 0 ) {
                    setMonsterCountOnTile( tile, monstersLeft );

                    if ( Maps::isMonsterOnTileJoinConditionFree( tile ) ) {
                        Maps::setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_MONEY );
                    }
                }
                else {
                    // TODO: do fading animation for the hero and for the monsters at the same time.
                    destroy = true;
                }
            }
        }

        if ( destroy ) {
            AudioManager::PlaySound( M82::KILLFADE );

            assert( tile.GetObject() == MP2::OBJ_MONSTER );

            runActionObjectFadeOutAnumation( tile, MP2::OBJ_MONSTER );

            resetObjectMetadata( tile );
        }

        // Clear the hero's attacked monster tile index
        hero.SetAttackedMonsterTileIndex( -1 );
    }

    void ActionToCastle( Heroes & hero, const int32_t dstIndex )
    {
        Castle * castle = world.getCastleEntrance( Maps::GetPoint( dstIndex ) );
        if ( castle == nullptr ) {
            // This should never happen
            assert( 0 );

            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       hero.GetName() << " is trying to visit the castle on tile " << dstIndex << ", but there is no entrance to the castle on this tile" )
            return;
        }

        if ( hero.GetColor() == castle->GetColor() ) {
            assert( hero.GetIndex() == dstIndex );

            DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " visits " << castle->GetName() )

            castle->MageGuildEducateHero( hero );
            Game::OpenCastleDialog( *castle );

            return;
        }

        if ( hero.isFriends( castle->GetColor() ) ) {
            // This should never happen - hero should not be able to visit an allied castle
            assert( 0 );

            DEBUG_LOG( DBG_GAME, DBG_WARN, hero.GetName() << " is not allowed to visit the allied castle " << castle->GetName() )
            return;
        }

        const auto captureCastle = [&hero, dstIndex, castle]() {
            Kingdom & enemyKingdom = castle->GetKingdom();
            enemyKingdom.RemoveCastle( castle );
            hero.GetKingdom().AddCastle( castle );
            world.CaptureObject( dstIndex, hero.GetColor() );

            castle->Scout();

            Interface::AdventureMap & I = Interface::AdventureMap::Get();

            // If the enemy is not vanquished we update only the area around the castle on radar.
            if ( !enemyKingdom.isLoss() ) {
                const int32_t scoutRange = static_cast<int32_t>( GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::CASTLE ) );
                const fheroes2::Point castlePosition = Maps::GetPoint( dstIndex );

                I.getRadar().SetRenderArea( { castlePosition.x - scoutRange, castlePosition.y - scoutRange, 2 * scoutRange + 1, 2 * scoutRange + 1 } );
            }
            // Otherwise we fully redraw the radar map image as there might be color reset of enemy's objects.
            I.setRedraw( Interface::REDRAW_CASTLES | Interface::REDRAW_RADAR );
        };

        Army & army = castle->GetActualArmy();

        // Hero is standing in front of the castle, which means that there must be an enemy army in the castle
        if ( hero.GetIndex() != dstIndex ) {
            assert( army.isValid() && army.GetColor() != hero.GetColor() );

            DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " attacks the enemy castle " << castle->GetName() )

            Heroes * defender = castle->GetHero();

            castle->ActionPreBattle();

            const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dstIndex );

            castle->ActionAfterBattle( res.AttackerWins() );

            // The defender was defeated
            if ( !res.DefenderWins() && defender ) {
                BattleLose( *defender, res, false );
            }

            // The attacker was defeated
            if ( !res.AttackerWins() ) {
                BattleLose( hero, res, true );
            }

            // The attacker won
            if ( res.AttackerWins() ) {
                captureCastle();

                hero.IncreaseExperience( res.GetExperienceAttacker() );
            }
            // The defender won
            else if ( res.DefenderWins() && defender ) {
                defender->IncreaseExperience( res.GetExperienceDefender() );
            }

            return;
        }

        // If hero is already in the castle, then there must be his own army in the castle
        assert( army.isValid() && army.GetColor() == hero.GetColor() );

        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " captures the enemy castle " << castle->GetName() )

        captureCastle();

        castle->MageGuildEducateHero( hero );
        Game::OpenCastleDialog( *castle );
    }

    void ActionToHeroes( Heroes & hero, const int32_t dstIndex )
    {
        Heroes * otherHero = world.GetTiles( dstIndex ).getHero();
        if ( otherHero == nullptr ) {
            // This should never happen
            assert( 0 );

            DEBUG_LOG( DBG_GAME, DBG_WARN, hero.GetName() << " is trying to meet the hero on tile " << dstIndex << ", but there is no hero on this tile" )
            return;
        }

        if ( hero.GetColor() == otherHero->GetColor() ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " meets " << otherHero->GetName() )

            hero.MeetingDialog( *otherHero );

            return;
        }

        if ( hero.isFriends( otherHero->GetColor() ) ) {
            // This should never happen - hero should not be able to meet an allied hero
            assert( 0 );

            DEBUG_LOG( DBG_GAME, DBG_WARN, hero.GetName() << " is not allowed to meet the allied hero " << otherHero->GetName() )
            return;
        }

        if ( otherHero->inCastle() ) {
            ActionToCastle( hero, dstIndex );

            return;
        }

        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " attacks " << otherHero->GetName() )

        const Battle::Result res = Battle::Loader( hero.GetArmy(), otherHero->GetArmy(), dstIndex );

        // TODO: make fading animation of both heroes together.

        // The defender was defeated
        if ( !res.DefenderWins() ) {
            BattleLose( *otherHero, res, false );
        }

        // The attacker was defeated
        if ( !res.AttackerWins() ) {
            BattleLose( hero, res, true );
        }

        // The attacker won
        if ( res.AttackerWins() ) {
            hero.IncreaseExperience( res.GetExperienceAttacker() );
        }
        // The defender won
        else if ( res.DefenderWins() ) {
            otherHero->IncreaseExperience( res.GetExperienceDefender() );
        }
    }

    void ActionToBoat( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        if ( hero.isShipMaster() ) {
            return;
        }

        hero.setLastGroundRegion( world.GetTiles( hero.GetIndex() ).GetRegion() );

        const fheroes2::Point offset( Maps::GetPoint( dst_index ) - hero.GetCenter() );

        // Get the direction of the boat so that the direction of the hero can be set to it after boarding
        const int boatDirection = world.GetTiles( dst_index ).getBoatDirection();

        AudioManager::PlaySound( M82::KILLFADE );
        hero.ShowPath( false );
        hero.FadeOut( offset );

        hero.Scout( dst_index );
        hero.Move2Dest( dst_index );
        hero.ResetMovePoints();
        hero.GetPath().Reset();

        // Update the radar map image before changing the direction of the hero.
        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        I.getRadar().SetRenderArea( hero.GetScoutRoi() );
        I.redraw( Interface::REDRAW_RADAR );

        // Set the direction of the hero to the one of the boat as the boat does not move when boarding it
        hero.setDirection( boatDirection );
        hero.setObjectTypeUnderHero( MP2::OBJ_NONE );
        world.GetTiles( dst_index ).resetObjectSprite();
        hero.SetShipMaster( true );

        hero.ShowPath( true );

        // Boat is no longer empty so we reset color to default
        world.GetTiles( dst_index ).resetBoatOwnerColor();
    }

    void ActionToCoast( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        if ( !hero.isShipMaster() ) {
            return;
        }

        const int fromIndex = hero.GetIndex();
        Maps::Tiles & from = world.GetTiles( fromIndex );

        // Calculate the offset before making the action.
        const fheroes2::Point offset( Maps::GetPoint( dst_index ) - hero.GetCenter() );

        hero.ShowPath( false );

        hero.Scout( dst_index );
        hero.Move2Dest( dst_index );
        hero.ResetMovePoints();
        hero.GetPath().Reset();

        from.setBoat( Maps::GetDirection( fromIndex, dst_index ), hero.GetColor() );
        hero.SetShipMaster( false );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn( offset );
        hero.ShowPath( true );

        // Clear hero position marker from the boat and scout the area on radar after disembarking.
        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        I.getRadar().SetRenderArea( hero.GetScoutRoi() );
        I.setRedraw( Interface::REDRAW_RADAR );

        hero.ActionNewPosition( true );
    }

    void ActionToPickupResource( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        Maps::Tiles & tile = world.GetTiles( dst_index );

        Interface::AdventureMap & I = Interface::AdventureMap::Get();

        if ( objectType == MP2::OBJ_BOTTLE ) {
            const MapSign * sign = dynamic_cast<MapSign *>( world.GetMapObject( dst_index ) );
            fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), ( sign ? sign->message : "No message provided" ), Dialog::OK );
        }
        else {
            const Funds funds = getFundsFromTile( tile );

            if ( objectType == MP2::OBJ_CAMPFIRE ) {
                const fheroes2::Text header( MP2::StringObject( objectType ), fheroes2::FontType::normalYellow() );
                const fheroes2::Text body( _( "Ransacking an enemy camp, you discover a hidden cache of treasures." ), fheroes2::FontType::normalWhite() );

                fheroes2::showResourceMessage( header, body, Dialog::OK, funds );
            }
            else {
                const auto resource = funds.getFirstValidResource();

                I.getStatusWindow().SetResource( resource.first, resource.second );
                I.setRedraw( Interface::REDRAW_STATUS );
            }

            hero.GetKingdom().AddFundsResource( funds );
        }

        Game::PlayPickupSound();

        runActionObjectFadeOutAnumation( tile, objectType );

        resetObjectMetadata( tile );
    }

    void ActionToObjectResource( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Funds funds = getFundsFromTile( tile );

        std::string msg;
        std::string caption = MP2::StringObject( objectType );

        switch ( objectType ) {
        case MP2::OBJ_WINDMILL:
            msg = funds.GetValidItemsCount() > 0
                      ? _(
                          "The keeper of the mill announces:\n\"Milord, I have been working very hard to provide you with these resources, come back next week for more.\"" )
                      : _( "The keeper of the mill announces:\n\"Milord, I am sorry, there are no resources currently available. Please try again next week.\"" );
            break;

        case MP2::OBJ_WATER_WHEEL:
            msg = funds.GetValidItemsCount() > 0
                      ? _( "The keeper of the mill announces:\n\"Milord, I have been working very hard to provide you with this gold, come back next week for more.\"" )
                      : _( "The keeper of the mill announces:\n\"Milord, I am sorry, there is no gold currently available. Please try again next week.\"" );
            break;

        case MP2::OBJ_LEAN_TO:
            msg = funds.GetValidItemsCount() > 0 ? _( "You've found an abandoned lean-to.\nPoking about, you discover some resources hidden nearby." )
                                                 : _( "The lean-to is long abandoned. There is nothing of value here." );
            break;

        case MP2::OBJ_MAGIC_GARDEN:
            msg = funds.GetValidItemsCount() > 0
                      ? _(
                          "You catch a leprechaun foolishly sleeping amidst a cluster of magic mushrooms.\nIn exchange for his freedom, he guides you to a small pot filled with precious things." )
                      : _(
                          "You've found a magic garden, the kind of place that leprechauns and faeries like to cavort in, but there is no one here today.\nPerhaps you should try again next week." );
            break;

        default:
            break;
        }

        if ( funds.GetValidItemsCount() > 0 ) {
            {
                const MusicalEffectPlayer musicalEffectPlayer;

                // The Magic Garden has a special sound
                if ( objectType == MP2::OBJ_MAGIC_GARDEN && MusicalEffectPlayer::canBePlayed( MUS::TREEHOUSE ) ) {
                    MusicalEffectPlayer::play( MUS::TREEHOUSE );
                }
                // The Lean-To has a special sound
                else if ( objectType == MP2::OBJ_LEAN_TO ) {
                    AudioManager::PlaySound( M82::EXPERNCE );
                }
                else {
                    AudioManager::PlaySound( M82::TREASURE );
                }

                fheroes2::showResourceMessage( fheroes2::Text( std::move( caption ), fheroes2::FontType::normalYellow() ),
                                               fheroes2::Text( std::move( msg ), fheroes2::FontType::normalWhite() ), Dialog::OK, funds );
            }

            hero.GetKingdom().AddFundsResource( funds );
        }
        else {
            fheroes2::showStandardTextMessage( std::move( caption ), std::move( msg ), Dialog::OK );
        }

        resetObjectMetadata( tile );
        hero.setVisitedForAllies( dst_index );
    }

    void ActionToSkeleton( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string message( _( "You come upon the remains of an unfortunate adventurer." ) );
        std::string title( MP2::StringObject( objectType ) );

        // artifact
        if ( doesTileContainValuableItems( tile ) ) {
            if ( hero.IsFullBagArtifacts() ) {
                const uint32_t gold = GoldInsteadArtifact( objectType );
                const Funds funds( Resource::GOLD, gold );
                AudioManager::PlaySound( M82::EXPERNCE );

                fheroes2::showResourceMessage( fheroes2::Text( std::move( title ), fheroes2::FontType::normalYellow() ),
                                               fheroes2::Text( _( "Treasure" ), fheroes2::FontType::normalWhite() ), Dialog::OK, funds );

                hero.GetKingdom().AddFundsResource( funds );
            }
            else {
                const Artifact & art = getArtifactFromTile( tile );
                message += '\n';
                message.append( _( "Searching through the tattered clothing, you find the %{artifact}." ) );
                StringReplace( message, "%{artifact}", art.GetName() );
                AudioManager::PlaySound( M82::TREASURE );

                const fheroes2::ArtifactDialogElement artifactUI( art );

                fheroes2::showStandardTextMessage( std::move( title ), std::move( message ), Dialog::OK, { &artifactUI } );

                hero.PickupArtifact( art );
            }

            resetObjectMetadata( tile );
        }
        else {
            message += '\n';
            message.append( _( "Searching through the tattered clothing, you find nothing." ) );
            fheroes2::showStandardTextMessage( std::move( title ), std::move( message ), Dialog::OK );
        }

        hero.SetVisitedWideTile( dst_index, objectType, Visit::GLOBAL );
    }

    void ActionToWagon( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string message( _( "You come across an old wagon left by a trader who didn't quite make it to safe terrain." ) );
        std::string title( MP2::StringObject( MP2::OBJ_WAGON ) );

        if ( doesTileContainValuableItems( tile ) ) {
            const Artifact & art = getArtifactFromTile( tile );

            if ( art.isValid() ) {
                if ( hero.IsFullBagArtifacts() ) {
                    message += '\n';
                    message.append( _( "Unfortunately, others have found it first, and the wagon is empty." ) );
                    fheroes2::showStandardTextMessage( std::move( title ), std::move( message ), Dialog::OK );
                }
                else {
                    message += '\n';
                    message.append( _( "Searching inside, you find the %{artifact}." ) );
                    StringReplace( message, "%{artifact}", art.GetName() );
                    AudioManager::PlaySound( M82::TREASURE );

                    const fheroes2::ArtifactDialogElement artifactUI( art );

                    fheroes2::showStandardTextMessage( std::move( title ), std::move( message ), Dialog::OK, { &artifactUI } );

                    hero.PickupArtifact( art );
                }
            }
            else {
                const Funds & funds = getFundsFromTile( tile );
                AudioManager::PlaySound( M82::EXPERNCE );
                message += '\n';
                message.append( _( "Inside, you find some of the wagon's cargo still intact." ) );

                fheroes2::showResourceMessage( fheroes2::Text( std::move( title ), fheroes2::FontType::normalYellow() ),
                                               fheroes2::Text( std::move( message ), fheroes2::FontType::normalWhite() ), Dialog::OK, funds );

                hero.GetKingdom().AddFundsResource( funds );
            }

            resetObjectMetadata( tile );
        }
        else {
            message += '\n';
            message.append( _( "Unfortunately, others have found it first, and the wagon is empty." ) );
            fheroes2::showStandardTextMessage( title, message, Dialog::OK );
        }

        hero.SetVisited( dst_index, Visit::GLOBAL );
    }

    void ActionToFlotSam( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string msg;
        std::string title( MP2::StringObject( objectType ) );

        const Funds & funds = getFundsFromTile( tile );

        if ( funds.GetValidItemsCount() > 0 ) {
            msg = funds.wood && funds.gold ? _( "You search through the flotsam, and find some wood and some gold." )
                                           : _( "You search through the flotsam, and find some wood." );

            fheroes2::showResourceMessage( fheroes2::Text( std::move( title ), fheroes2::FontType::normalYellow() ),
                                           fheroes2::Text( std::move( msg ), fheroes2::FontType::normalWhite() ), Dialog::OK, funds );

            hero.GetKingdom().AddFundsResource( funds );
        }
        else {
            msg = _( "You search through the flotsam, but find nothing." );
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
        }

        Game::PlayPickupSound();

        runActionObjectFadeOutAnumation( tile, objectType );

        resetObjectMetadata( tile );
    }

    void ActionToShrine( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const Spell & spell = getSpellFromTile( world.GetTiles( dst_index ) );
        assert( spell.isValid() );

        const int spellLevel = spell.Level();

        std::string head;
        std::string body;

        switch ( spellLevel ) {
        case 1:
            head = _( "Shrine of the 1st Circle" );
            body = _(
                "You come across a small shrine attended by a group of novice acolytes.\nIn exchange for your protection, they agree to teach you a simple spell - '%{spell}'." );
            break;
        case 2:
            head = _( "Shrine of the 2nd Circle" );
            body = _(
                "You come across an ornate shrine attended by a group of rotund friars.\nIn exchange for your protection, they agree to teach you a spell - '%{spell}'." );
            break;
        case 3:
            head = _( "Shrine of the 3rd Circle" );
            body = _(
                "You come across a lavish shrine attended by a group of high priests.\nIn exchange for your protection, they agree to teach you a sophisticated spell - '%{spell}'." );
            break;
        default:
            // Did you add a new level shrine? Add the logic above.
            assert( 0 );
            return;
        }

        StringReplace( body, "%{spell}", spell.GetName() );

        if ( hero.HaveSpellBook() ) {
            // check valid level spell and wisdom skill
            if ( 3 == spellLevel && Skill::Level::NONE == hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) {
                body += _( "\nUnfortunately, you do not have the wisdom to understand the spell, and you are unable to learn it." );
                fheroes2::showStandardTextMessage( std::move( head ), std::move( body ), Dialog::OK );
            }
            // already know (skip bag artifacts)
            else if ( hero.HaveSpell( spell.GetID(), true ) ) {
                body += _( "\nUnfortunately, you already have knowledge of this spell, so there is nothing more for them to teach you." );
                fheroes2::showStandardTextMessage( std::move( head ), std::move( body ), Dialog::OK );
            }
            else {
                AudioManager::PlaySound( M82::TREASURE );
                hero.AppendSpellToBook( spell.GetID() );

                const fheroes2::SpellDialogElement spellUI( spell, &hero );
                fheroes2::showStandardTextMessage( std::move( head ), std::move( body ), Dialog::OK, { &spellUI } );
            }
        }
        else {
            body += _( "\nUnfortunately, you have no Magic Book to record the spell with." );
            fheroes2::showStandardTextMessage( std::move( head ), std::move( body ), Dialog::OK );
        }

        hero.SetVisited( dst_index, Visit::GLOBAL );
    }

    void ActionToWitchsHut( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const Skill::Secondary & skill = getSecondarySkillFromWitchsHut( world.GetTiles( dst_index ) );
        if ( skill.isValid() ) {
            std::string msg = _( "You approach the hut and observe a witch inside studying an ancient tome on %{skill}.\n\n" );
            const std::string & skill_name = Skill::Secondary::String( skill.Skill() );
            StringReplace( msg, "%{skill}", skill_name );

            std::string title( MP2::StringObject( objectType ) );

            // No room for a new skill
            if ( hero.HasMaxSecondarySkill() ) {
                msg.append( _(
                    "As you approach, she turns and focuses her one glass eye on you.\n\"You already know everything you deserve to learn!\" the witch screeches. \"NOW GET OUT OF MY HOUSE!\"" ) );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
            }
            // Skill has already been learned
            else if ( hero.HasSecondarySkill( skill.Skill() ) ) {
                msg.append( _( "As you approach, she turns and speaks.\n\"You already know that which I would teach you. I can help you no further.\"" ) );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
            }
            else {
                hero.LearnSkill( skill );

                // When Scouting skill is learned we reveal the fog and redraw the radar map image in a new scout area of the hero.
                if ( skill.Skill() == Skill::Secondary::SCOUTING ) {
                    hero.Scout( hero.GetIndex() );
                    hero.ScoutRadar();
                }

                msg.append( _( "An ancient and immortal witch living in a hut with bird's legs for stilts teaches you %{skill} for her own inscrutable purposes." ) );
                StringReplace( msg, "%{skill}", skill_name );

                {
                    const MusicalEffectPlayer musicalEffectPlayer( MUS::EXPERIENCE );

                    const fheroes2::SecondarySkillDialogElement secondarySkillUI( skill, hero );
                    fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &secondarySkillUI } );
                }
            }
        }
        else {
            // A broken object?
            assert( 0 );
        }

        hero.SetVisited( dst_index, Visit::GLOBAL );
    }

    void ActionToGoodLuckObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        const bool visited = hero.isObjectTypeVisited( objectType );
        std::string msg;

        switch ( objectType ) {
        case MP2::OBJ_FOUNTAIN:
            msg = visited ? _( "You drink from the enchanted fountain, but nothing happens." ) : _( "As you drink the sweet water, you gain luck for your next battle." );
            break;

        case MP2::OBJ_FAERIE_RING:
            msg = visited ? _( "You enter the faerie ring, but nothing happens." )
                          : _( "Upon entering the mystical faerie ring, your army gains luck for its next battle." );
            break;

        case MP2::OBJ_IDOL:
            msg = visited ? _(
                      "You've found an ancient and weathered stone idol.\nIt is supposed to grant luck to visitors, but since the stars are already smiling upon you, it does nothing." )
                          : _( "You've found an ancient and weathered stone idol.\nKissing it is supposed to be lucky, so you do. The stone is very cold to the touch." );
            break;

        case MP2::OBJ_MERMAID:
            msg = visited
                      ? _( "The mermaids silently entice you to return later and be blessed again." )
                      : _(
                          "The magical, soothing beauty of the Mermaids reaches you and your crew.\nJust for a moment, you forget your worries and bask in the beauty of the moment.\nThe mermaids' charms bless you with increased luck for your next combat." );
            break;

        default:
            break;
        }

        std::string title( MP2::StringObject( objectType ) );

        // check already visited
        if ( visited ) {
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
        }
        else {
            // modify luck
            hero.SetVisited( dst_index );
            AudioManager::PlaySound( M82::GOODLUCK );

            const fheroes2::LuckDialogElement luckUI( true );
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &luckUI } );
        }
    }

    void ActionToPyramid( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Spell & spell = getSpellFromTile( tile );

        std::string ask = _(
            "You come upon the pyramid of a great and ancient king.\nYou are tempted to search it for treasure, but all the old stories warn of fearful curses and undead "
            "guardians.\nWill you search?" );
        std::string title( MP2::StringObject( objectType ) );

        bool enter = false;

        {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::DUNGEON );

            enter = ( fheroes2::showStandardTextMessage( title, std::move( ask ), Dialog::YES | Dialog::NO ) == Dialog::YES );
        }

        if ( enter ) {
            if ( spell.isValid() ) {
                // battle
                Army army( tile );

                const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
                if ( res.AttackerWins() ) {
                    hero.IncreaseExperience( res.GetExperienceAttacker() );
                    bool valid = false;

                    std::string msg = _( "Upon defeating the monsters, you decipher an ancient glyph on the wall, telling the secret of the spell - '" );
                    msg += spell.GetName();
                    msg += "'.";

                    // Check Magic Book.
                    if ( !hero.HaveSpellBook() ) {
                        msg += '\n';
                        msg += _( "Unfortunately, you have no Magic Book to record the spell with." );
                    }
                    else if ( Skill::Level::EXPERT > hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) {
                        // check skill level for wisdom
                        msg += '\n';
                        msg += _( "Unfortunately, you do not have the wisdom to understand the spell, and you are unable to learn it." );
                    }
                    else {
                        valid = true;
                    }

                    if ( valid ) {
                        const fheroes2::SpellDialogElement spellUI( spell, &hero );
                        fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &spellUI } );

                        hero.AppendSpellToBook( spell );
                    }
                    else {
                        fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
                    }

                    resetObjectMetadata( tile );
                    hero.SetVisited( dst_index, Visit::GLOBAL );
                }
                else {
                    BattleLose( hero, res, true );
                }
            }
            else {
                // Modify luck
                AudioManager::PlaySound( M82::BADLUCK );

                std::string msg = _( "You come upon the pyramid of a great and ancient king.\nRoutine exploration reveals that the pyramid is completely empty." );

                const fheroes2::LuckDialogElement luckUI( false );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &luckUI, &luckUI } );

                hero.SetVisited( dst_index, Visit::LOCAL );
                hero.SetVisited( dst_index, Visit::GLOBAL );
            }
        }
    }

    void ActionToSign( const Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )
#ifndef WITH_DEBUG
        (void)hero;
#endif

        const MapSign * sign = dynamic_cast<MapSign *>( world.GetMapObject( dst_index ) );
        fheroes2::showStandardTextMessage( MP2::StringObject( MP2::OBJ_SIGN ), ( sign ? sign->message : "" ), Dialog::OK );
    }

    void ActionToMagicWell( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const uint32_t max = hero.GetMaxSpellPoints();
        std::string title( MP2::StringObject( MP2::OBJ_MAGIC_WELL ) );

        if ( hero.GetSpellPoints() >= max ) {
            fheroes2::showStandardTextMessage( std::move( title ), _( "A drink at the well is supposed to restore your spell points, but you are already at maximum." ),
                                               Dialog::OK );
        }
        else {
            if ( hero.isObjectTypeVisited( MP2::OBJ_MAGIC_WELL ) ) {
                const MusicalEffectPlayer musicalEffectPlayer( MUS::WATERSPRING );

                fheroes2::showStandardTextMessage( std::move( title ), _( "A second drink at the well in one day will not help you." ), Dialog::OK );
            }
            else {
                hero.SetSpellPoints( max );

                {
                    const MusicalEffectPlayer musicalEffectPlayer( MUS::WATERSPRING );

                    fheroes2::showStandardTextMessage( std::move( title ), _( "A drink from the well has restored your spell points to maximum." ), Dialog::OK );
                }

                hero.SetVisited( dst_index );
            }
        }
    }

    void ActionToTradingPost( const Heroes & hero )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Dialog::Marketplace( hero.GetKingdom(), true );
    }

    void ActionToPrimarySkillObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        const Maps::Tiles & tile = world.GetTiles( dst_index );

        std::string msg;
        int skill = Skill::Primary::ATTACK;
        const bool visited = hero.isVisited( tile );

        switch ( objectType ) {
        case MP2::OBJ_FORT:
            skill = Skill::Primary::DEFENSE;
            msg = visited ? _( "\"I'm sorry sir,\" The leader of the soldiers says, \"but you already know everything we have to teach.\"" )
                          : _( "The soldiers living in the fort teach you a few new defensive tricks." );
            break;

        case MP2::OBJ_MERCENARY_CAMP:
            skill = Skill::Primary::ATTACK;
            msg = visited ? _(
                      "You've come upon a mercenary camp practicing their tactics. \"You're too advanced for us,\" the mercenary captain says. \"We can teach nothing more.\"" )
                          : _(
                              "You've come upon a mercenary camp practicing their tactics. The mercenaries welcome you and your troops and invite you to train with them." );
            break;

        case MP2::OBJ_WITCH_DOCTORS_HUT:
            skill = Skill::Primary::KNOWLEDGE;
            msg = visited
                      ? _( "\"Go 'way!\", the witch doctor barks, \"you know all I know.\"" )
                      : _(
                          "An Orcish witch doctor living in the hut deepens your knowledge of magic by showing you how to cast stones, read portents, and decipher the intricacies of chicken entrails." );
            break;

        case MP2::OBJ_STANDING_STONES:
            skill = Skill::Primary::POWER;
            msg = visited ? _(
                      "You've found a group of Druids worshipping at one of their strange stone edifices. Silently, the Druids turn you away, indicating they have nothing new to teach you." )
                          : _( "You've found a group of Druids worshipping at one of their strange stone edifices. Silently, they teach you new ways to cast spells." );
            break;

        default:
            return;
        }

        std::string title( MP2::StringObject( objectType ) );

        if ( visited ) {
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
        }
        else {
            hero.IncreasePrimarySkill( skill );

            {
                const MusicalEffectPlayer musicalEffectPlayer( MUS::SKILL );

                const fheroes2::PrimarySkillDialogElement primarySkillUI( skill, "+1" );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &primarySkillUI } );
            }

            hero.SetVisited( dst_index );
            hero.SetVisitedWideTile( dst_index, objectType );
        }
    }

    void ActionToPoorMoraleObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Funds funds = getFundsFromTile( tile );
        assert( funds.GetValidItemsCount() == 0 || ( funds.GetValidItemsCount() == 1 && funds.gold > 0 ) );

        uint32_t gold = funds.gold;
        std::string ask;
        std::string msg;
        std::string win;

        switch ( objectType ) {
        case MP2::OBJ_GRAVEYARD:
            ask = _( "You tentatively approach the burial ground of ancient warriors. Do you want to search the graves?" );
            msg = _( "You spend several hours searching the graves and find nothing. Such a despicable act reduces your army's morale." );
            win = _( "Upon defeating the Zombies you search the graves and find something!" );
            break;
        case MP2::OBJ_SHIPWRECK:
            ask = _( "The rotting hulk of a great pirate ship creaks eerily as it is pushed against the rocks. Do you wish to search the shipwreck?" );
            msg = _( "You spend several hours sifting through the debris and find nothing. Such a despicable act reduces your army's morale." );
            win = _( "Upon defeating the Ghosts you sift through the debris and find something!" );
            break;
        case MP2::OBJ_DERELICT_SHIP:
            ask = _( "The rotting hulk of a great pirate ship creaks eerily as it is pushed against the rocks. Do you wish to search the ship?" );
            msg = _( "You spend several hours sifting through the debris and find nothing. Such a despicable act reduces your army's morale." );
            win = _( "Upon defeating the Skeletons you sift through the debris and find something!" );
            break;
        default:
            break;
        }

        const std::string title( MP2::StringObject( objectType ) );
        {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::WATCHTOWER );

            if ( fheroes2::showStandardTextMessage( title, std::move( ask ), Dialog::YES | Dialog::NO ) != Dialog::YES ) {
                return;
            }
        }

        bool complete = false;

        if ( gold ) {
            Army army( tile );

            const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );

                complete = true;

                Artifact art;

                if ( MP2::isArtifactObject( objectType ) ) {
                    art = getArtifactFromTile( tile );
                }

                if ( art.isValid() ) {
                    if ( hero.IsFullBagArtifacts() ) {
                        gold = GoldInsteadArtifact( objectType );

                        const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

                        fheroes2::showStandardTextMessage( title, std::move( win ), Dialog::OK, { &goldUI } );
                    }
                    else {
                        const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );
                        const fheroes2::ArtifactDialogElement artifactUI( art );

                        fheroes2::showStandardTextMessage( title, std::move( win ), Dialog::OK, { &artifactUI, &goldUI } );

                        hero.PickupArtifact( art );
                    }
                }
                else {
                    const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

                    fheroes2::showStandardTextMessage( title, std::move( win ), Dialog::OK, { &goldUI } );
                }

                hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, gold ) );
            }
            else {
                BattleLose( hero, res, true );
            }
        }

        if ( complete ) {
            resetObjectMetadata( tile );
            hero.SetVisited( dst_index, Visit::GLOBAL );
        }
        else if ( 0 == gold ) {
            // Modify morale
            hero.SetVisited( dst_index, Visit::LOCAL );
            hero.SetVisited( dst_index, Visit::GLOBAL );

            AudioManager::PlaySound( M82::BADMRLE );

            const fheroes2::MoraleDialogElement moraleUI( false );
            fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::OK, { &moraleUI } );
        }
    }

    void ActionToGoodMoraleObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        std::string msg;
        uint32_t move = 0;
        const bool visited = hero.isObjectTypeVisited( objectType );

        switch ( objectType ) {
        case MP2::OBJ_BUOY:
            msg = visited ? _( "Your men spot a navigational buoy, confirming that you are on course." )
                          : _( "Your men spot a navigational buoy, confirming that you are on course and increasing their morale." );
            break;

        case MP2::OBJ_OASIS:
            msg = visited ? _( "The drink at the oasis is refreshing, but offers no further benefit. The oasis might help again if you fought a battle first." )
                          : _( "A drink at the oasis fills your troops with strength and lifts their spirits. You can travel a bit further today." );
            move = GameStatic::getMovementPointBonus( MP2::OBJ_OASIS );
            break;

        case MP2::OBJ_WATERING_HOLE:
            msg = visited ? _(
                      "The drink at the watering hole is refreshing, but offers no further benefit. The watering hole might help again if you fought a battle first." )
                          : _( "A drink at the watering hole fills your troops with strength and lifts their spirits. You can travel a bit further today." );
            move = GameStatic::getMovementPointBonus( MP2::OBJ_WATERING_HOLE );
            break;

        case MP2::OBJ_TEMPLE:
            msg = visited ? _( "It doesn't help to pray twice before a battle. Come back after you've fought." )
                          : _( "A visit and a prayer at the temple raises the morale of your troops." );
            break;

        default:
            return;
        }

        std::string title( MP2::StringObject( objectType ) );
        // check already visited
        if ( visited ) {
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
        }
        else {
            // modify morale
            hero.SetVisited( dst_index );
            AudioManager::PlaySound( M82::GOODMRLE );

            const fheroes2::MoraleDialogElement moraleUI( true );
            std::vector<const fheroes2::DialogElement *> elementUI{ &moraleUI };
            if ( objectType == MP2::OBJ_TEMPLE ) {
                elementUI.emplace_back( &moraleUI );
            }

            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, elementUI );

            hero.IncreaseMovePoints( move );

            // fix double action tile
            hero.SetVisitedWideTile( dst_index, objectType );
        }
    }

    void ActionToExperienceObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        const Maps::Tiles & tile = world.GetTiles( dst_index );

        const bool visited = hero.isVisited( tile );
        std::string msg;

        int32_t exp = 0;

        switch ( objectType ) {
        case MP2::OBJ_GAZEBO:
            msg = visited ? _( "An old Knight appears on the steps of the gazebo. \"I am sorry, my liege, I have taught you all I can.\"" )
                          : _( "An old Knight appears on the steps of the gazebo. \"My liege, I will teach you all that I know to aid you in your travels.\"" );
            exp = 1000;
            break;

        default:
            // Are you calling this function for a new object? Add the logic above!
            assert( 0 );
            return;
        }

        std::string title( MP2::StringObject( objectType ) );

        if ( visited ) {
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
        }
        else {
            {
                const MusicalEffectPlayer musicalEffectPlayer;

                if ( MusicalEffectPlayer::canBePlayed( MUS::EXPERIENCE ) ) {
                    MusicalEffectPlayer::play( MUS::EXPERIENCE );
                }
                else {
                    AudioManager::PlaySound( M82::EXPERNCE );
                }

                const fheroes2::ExperienceDialogElement experienceUI( exp );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &experienceUI } );
            }

            hero.IncreaseExperience( exp );
            hero.SetVisited( dst_index );
        }
    }

    void ActionToShipwreckSurvivor( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );

        std::string title( MP2::StringObject( objectType ) );

        if ( hero.IsFullBagArtifacts() ) {
            const uint32_t gold = GoldInsteadArtifact( objectType );

            const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

            fheroes2::showStandardTextMessage( std::move( title ),
                                               _( "You've pulled a shipwreck survivor from certain death in an unforgiving ocean. Grateful, he says, "
                                                  "\"I would give you an artifact as a reward, but you're all full.\"" ),
                                               Dialog::OK, { &goldUI } );

            hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, gold ) );
        }
        else {
            const Artifact & art = getArtifactFromTile( tile );
            std::string str = _(
                "You've pulled a shipwreck survivor from certain death in an unforgiving ocean. Grateful, he rewards you for your act of kindness by giving you the %{art}." );
            StringReplace( str, "%{art}", art.GetName() );
            AudioManager::PlaySound( M82::TREASURE );

            const fheroes2::ArtifactDialogElement artifactUI( art );

            fheroes2::showStandardTextMessage( std::move( title ), std::move( str ), Dialog::OK, { &artifactUI } );

            hero.PickupArtifact( art );
        }

        Game::PlayPickupSound();

        runActionObjectFadeOutAnumation( tile, objectType );

        resetObjectMetadata( tile );
    }

    void ActionToArtifact( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string title( MP2::StringObject( MP2::OBJ_ARTIFACT ) );

        const Artifact art = getArtifactFromTile( tile );
        if ( art.GetID() == Artifact::MAGIC_BOOK && hero.HaveSpellBook() ) {
            fheroes2::showStandardTextMessage( std::move( title ), _( "You cannot have multiple spell books." ), Dialog::OK );

            return;
        }

        if ( hero.IsFullBagArtifacts() ) {
            fheroes2::showStandardTextMessage( std::move( title ), _( "You cannot pick up this artifact, you already have a full load!" ), Dialog::OK );

            return;
        }

        const Maps::ArtifactCaptureCondition condition = getArtifactCaptureCondition( tile );

        bool result = false;
        std::string msg;

        if ( condition == Maps::ArtifactCaptureCondition::PAY_2000_GOLD || condition == Maps::ArtifactCaptureCondition::PAY_2500_GOLD_AND_3_RESOURCES
             || condition == Maps::ArtifactCaptureCondition::PAY_3000_GOLD_AND_5_RESOURCES ) {
            const Funds payment = getArtifactResourceRequirement( tile );

            if ( condition == Maps::ArtifactCaptureCondition::PAY_2000_GOLD ) {
                msg = _( "A leprechaun offers you the %{art} for the small price of %{gold} Gold." );
                StringReplace( msg, "%{gold}", payment.gold );
            }
            else {
                msg = _( "A leprechaun offers you the %{art} for the small price of %{gold} Gold and %{count} %{res}." );

                StringReplace( msg, "%{gold}", payment.gold );

                for ( const int res : { Resource::WOOD, Resource::MERCURY, Resource::ORE, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS } ) {
                    const uint32_t count = payment.Get( res );
                    if ( count > 0 ) {
                        StringReplace( msg, "%{res}", Resource::String( res ) );
                        StringReplace( msg, "%{count}", static_cast<int>( count ) );
                        break;
                    }
                }
            }
            StringReplace( msg, "%{art}", art.GetName() );
            msg += '\n';
            msg.append( _( "Do you wish to buy this artifact?" ) );

            AudioManager::PlaySound( M82::EXPERNCE );

            const fheroes2::ArtifactDialogElement artifactUI( art );

            if ( Dialog::YES == fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::YES | Dialog::NO, { &artifactUI } ) ) {
                if ( hero.GetKingdom().AllowPayment( payment ) ) {
                    result = true;
                    hero.GetKingdom().OddFundsResource( payment );
                }
                else {
                    fheroes2::showStandardTextMessage(
                        std::move( title ), _( "You try to pay the leprechaun, but realize that you can't afford it. The leprechaun stamps his foot and ignores you." ),
                        Dialog::OK );
                }
            }
            else {
                fheroes2::showStandardTextMessage( std::move( title ),
                                                   _( "Insulted by your refusal of his generous offer, the leprechaun stamps his foot and ignores you." ), Dialog::OK );
            }
        }
        else if ( condition == Maps::ArtifactCaptureCondition::HAVE_WISDOM_SKILL || condition == Maps::ArtifactCaptureCondition::HAVE_LEADERSHIP_SKILL ) {
            const Skill::Secondary & skill = getArtifactSecondarySkillRequirement( tile );

            if ( hero.HasSecondarySkill( skill.Skill() ) ) {
                const char * artifactDiscoveryDescription = Artifact::getDiscoveryDescription( art );
                if ( artifactDiscoveryDescription != nullptr ) {
                    msg = artifactDiscoveryDescription;
                }
                else {
                    msg = _( "You've found the artifact: " );
                    msg += '\n';
                    msg.append( art.GetName() );
                }
                AudioManager::PlaySound( M82::TREASURE );

                const fheroes2::ArtifactDialogElement artifactUI( art );

                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &artifactUI } );

                result = true;
            }
            else {
                if ( skill.Skill() == Skill::Secondary::WISDOM ) {
                    msg = _(
                        "You've found the humble dwelling of a withered hermit. The hermit tells you that he is willing to give the %{art} to the first wise person he meets." );
                }
                else if ( skill.Skill() == Skill::Secondary::LEADERSHIP ) {
                    msg = _(
                        "You've come across the spartan quarters of a retired soldier. The soldier tells you that he is willing to pass on the %{art} to the first true leader he meets." );
                }
                else {
                    // Did you add a new condition? If yes add a proper if-else branch.
                    assert( 0 );
                    msg = _( "You've encountered a strange person with a hat and an owl on it. He tells you that he is willing to give %{art} if you have %{skill}." );
                    StringReplace( msg, "%{skill}", skill.GetName() );
                }

                StringReplace( msg, "%{art}", art.GetName() );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
            }
        }
        else if ( condition >= Maps::ArtifactCaptureCondition::FIGHT_50_ROGUES && condition <= Maps::ArtifactCaptureCondition::FIGHT_1_BONE_DRAGON ) {
            bool battle = true;
            Army army( tile );
            const Troop * troop = army.GetFirstValid();

            if ( troop ) {
                if ( Monster::ROGUE == troop->GetID() )
                    fheroes2::showStandardTextMessage(
                        title, _( "You come upon an ancient artifact. As you reach for it, a pack of Rogues leap out of the brush to guard their stolen loot." ),
                        Dialog::OK );
                else {
                    msg = _(
                        "Through a clearing you observe an ancient artifact. Unfortunately, it's guarded by a nearby %{monster}. Do you want to fight the %{monster} for the artifact?" );
                    StringReplaceWithLowercase( msg, "%{monster}", troop->GetName() );
                    battle = ( Dialog::YES == fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::YES | Dialog::NO ) );
                }
            }

            if ( battle ) {
                const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
                if ( res.AttackerWins() ) {
                    hero.IncreaseExperience( res.GetExperienceAttacker() );
                    result = true;
                    msg = _( "Victorious, you take your prize, the %{art}." );
                    StringReplace( msg, "%{art}", art.GetName() );
                    AudioManager::PlaySound( M82::TREASURE );

                    const fheroes2::ArtifactDialogElement artifactUI( art );

                    fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &artifactUI } );
                }
                else {
                    BattleLose( hero, res, true );
                }
            }
            else {
                fheroes2::showStandardTextMessage( std::move( title ), _( "Discretion is the better part of valor, and you decide to avoid this fight for today." ),
                                                   Dialog::OK );
            }
        }
        else {
            const char * artifactDiscoveryDescription = Artifact::getDiscoveryDescription( art );
            if ( artifactDiscoveryDescription != nullptr ) {
                msg = artifactDiscoveryDescription;
            }
            else {
                msg = _( "You've found the artifact: " );
                msg += '\n';
                msg.append( art.GetName() );
            }
            AudioManager::PlaySound( M82::TREASURE );

            const fheroes2::ArtifactDialogElement artifactUI( art );
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK, { &artifactUI } );
            result = true;
        }

        if ( result && hero.PickupArtifact( art ) ) {
            Game::PlayPickupSound();

            assert( tile.GetObject() == MP2::OBJ_ARTIFACT );

            runActionObjectFadeOutAnumation( tile, MP2::OBJ_ARTIFACT );

            resetObjectMetadata( tile );
        }
    }

    void ActionToTreasureChest( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string hdr = MP2::StringObject( objectType );

        std::string msg;
        const Funds funds = getFundsFromTile( tile );
        assert( funds.gold > 0 || funds.GetValidItemsCount() == 0 );

        uint32_t gold = funds.gold;

        // dialog
        if ( tile.isWater() ) {
            if ( gold ) {
                const Artifact & art = getArtifactFromTile( tile );

                if ( art.isValid() ) {
                    if ( hero.IsFullBagArtifacts() ) {
                        gold = GoldInsteadArtifact( objectType );
                        msg = _( "After spending hours trying to fish the chest out of the sea, you open it and find %{gold} gold pieces." );
                        StringReplace( msg, "%{gold}", gold );

                        const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

                        fheroes2::showStandardTextMessage( std::move( hdr ), std::move( msg ), Dialog::OK, { &goldUI } );
                    }
                    else {
                        msg = _( "After spending hours trying to fish the chest out of the sea, you open it and find %{gold} gold and the %{art}." );
                        StringReplace( msg, "%{gold}", gold );
                        StringReplace( msg, "%{art}", art.GetName() );

                        const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );
                        const fheroes2::ArtifactDialogElement artifactUI( art );

                        fheroes2::showStandardTextMessage( std::move( hdr ), std::move( msg ), Dialog::OK, { &artifactUI, &goldUI } );

                        hero.PickupArtifact( art );
                    }
                }
                else {
                    msg = _( "After spending hours trying to fish the chest out of the sea, you open it and find %{gold} gold pieces." );
                    StringReplace( msg, "%{gold}", gold );

                    const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

                    fheroes2::showStandardTextMessage( std::move( hdr ), std::move( msg ), Dialog::OK, { &goldUI } );
                }
            }
            else {
                fheroes2::showStandardTextMessage( std::move( hdr ),
                                                   _( "After spending hours trying to fish the chest out of the sea, you open it, only to find it empty." ), Dialog::OK );
            }
        }
        else {
            const Artifact & art = getArtifactFromTile( tile );

            if ( gold ) {
                const uint32_t expr = gold > 500 ? gold - 500 : 500;
                msg = _(
                    "After scouring the area, you fall upon a hidden treasure cache. You may take the gold or distribute the gold to the peasants for experience. Do you wish to keep the gold?" );

                if ( !Dialog::SelectGoldOrExp( hdr, msg, gold, expr, hero ) ) {
                    gold = 0;
                    hero.IncreaseExperience( expr );
                }
            }
            else if ( art.isValid() ) {
                if ( hero.IsFullBagArtifacts() ) {
                    gold = GoldInsteadArtifact( objectType );
                    msg = _( "After scouring the area, you fall upon a hidden chest, containing the %{gold} gold pieces." );
                    StringReplace( msg, "%{gold}", gold );

                    const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

                    fheroes2::showStandardTextMessage( std::move( hdr ), std::move( msg ), Dialog::OK, { &goldUI } );
                }
                else {
                    msg = _( "After scouring the area, you fall upon a hidden chest, containing the ancient artifact %{art}." );
                    StringReplace( msg, "%{art}", art.GetName() );
                    AudioManager::PlaySound( M82::TREASURE );

                    const fheroes2::ArtifactDialogElement artifactUI( art );

                    fheroes2::showStandardTextMessage( std::move( hdr ), std::move( msg ), Dialog::OK, { &artifactUI } );

                    hero.PickupArtifact( art );
                }
            }
        }

        if ( gold ) {
            hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, gold ) );
        }

        Game::PlayPickupSound();

        runActionObjectFadeOutAnumation( tile, objectType );

        resetObjectMetadata( tile );
    }

    void ActionToGenieLamp( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Troop & troop = getTroopFromTile( tile );
        if ( !troop.isValid() ) {
            return;
        }

        const std::string title( MP2::StringObject( objectType ) );

        bool recruit = false;

        {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::ARABIAN );

            recruit
                = ( fheroes2::showStandardTextMessage( title, _( "You stumble upon a dented and tarnished lamp lodged deep in the earth. Do you wish to rub the lamp?" ),
                                                       Dialog::YES | Dialog::NO )
                    == Dialog::YES );
        }

        if ( recruit ) {
            RecruitMonsterFromTile( hero, tile, title, troop, true );
        }
    }

    void ActionToTeleports( Heroes & hero, int32_t index_from )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const int32_t index_to = world.NextTeleport( index_from );
        if ( index_from == index_to ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, hero.GetName() << " has nowhere to go through stone liths" )

            AudioManager::PlaySound( M82::RSBRYFZL );
            return;
        }

        assert( world.GetTiles( index_to ).GetObject() != MP2::OBJ_HERO );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.ShowPath( false );
        hero.FadeOut();

        const fheroes2::Point fromPoint = Maps::GetPoint( index_from );

        hero.Scout( index_to );
        hero.Move2Dest( index_to );
        hero.GetPath().Reset();

        // Clear the previous hero position
        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        I.getRadar().SetRenderArea( { fromPoint.x, fromPoint.y, 1, 1 } );
        I.redraw( Interface::REDRAW_RADAR );

        I.getGameArea().SetCenter( hero.GetCenter() );

        I.getRadar().SetRenderArea( hero.GetScoutRoi() );
        I.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.ShowPath( true );

        hero.ActionNewPosition( false );
    }

    void ActionToWhirlpools( Heroes & hero, int32_t index_from )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const int32_t index_to = world.NextWhirlpool( index_from );
        if ( index_from == index_to ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, hero.GetName() << " has nowhere to go through the whirlpool" )

            AudioManager::PlaySound( M82::RSBRYFZL );
            return;
        }

        AudioManager::PlaySound( M82::KILLFADE );
        hero.ShowPath( false );
        hero.FadeOut();

        const fheroes2::Point fromPoint = Maps::GetPoint( index_from );

        hero.Scout( index_to );
        hero.Move2Dest( index_to );
        hero.GetPath().Reset();

        WhirlpoolTroopLoseEffect( hero );

        // Clear the previous hero position
        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        I.getRadar().SetRenderArea( { fromPoint.x, fromPoint.y, 1, 1 } );
        I.redraw( Interface::REDRAW_RADAR );

        I.getGameArea().SetCenter( hero.GetCenter() );

        I.getRadar().SetRenderArea( hero.GetScoutRoi() );
        I.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.ShowPath( true );

        hero.ActionNewPosition( false );
    }

    void ActionToCaptureObject( Heroes & hero, const MP2::MapObjectType objectType, const int32_t dstIndex )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << " object: " << MP2::StringObject( objectType ) )

        Maps::Tiles & tile = world.GetTiles( dstIndex );

        if ( !hero.isFriends( getColorFromTile( tile ) ) ) {
            const auto updateRadar = [objectType, dstIndex]() {
                // TODO: make a function that will automatically get the object size in tiles and return a ROI for radar update.
                // Set the radar update ROI according to captured object size and position.
                fheroes2::Rect radarRoi( Maps::GetPoint( dstIndex ), { 1, 1 } );

                switch ( objectType ) {
                case MP2::OBJ_SAWMILL:
                    radarRoi.x -= 2;
                    --radarRoi.y;
                    radarRoi.width = 4;
                    radarRoi.height = 2;
                    break;
                case MP2::OBJ_ALCHEMIST_LAB:
                case MP2::OBJ_MINE:
                    --radarRoi.x;
                    --radarRoi.y;
                    radarRoi.width = 3;
                    radarRoi.height = 2;
                    break;
                case MP2::OBJ_LIGHTHOUSE:
                    radarRoi.y -= 2;
                    radarRoi.height = 3;
                    break;
                default:
                    break;
                }

                Interface::AdventureMap & I = Interface::AdventureMap::Get();

                // Update the object on radar.
                I.getRadar().SetRenderArea( radarRoi );
                I.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );
            };

            const auto removeObjectProtection = [&tile]() {
                // Clear any metadata related to spells
                if ( tile.GetObject( false ) == MP2::OBJ_MINE ) {
                    removeMineSpellFromTile( tile );
                }
            };

            const auto captureObject = [&hero, objectType, &tile, &updateRadar, &removeObjectProtection]() {
                removeObjectProtection();

                setColorOnTile( tile, hero.GetColor() );

                updateRadar();

                std::string header;
                std::string body;
                int32_t resource = Resource::UNKNOWN;

                switch ( objectType ) {
                case MP2::OBJ_ALCHEMIST_LAB:
                    resource = Resource::MERCURY;
                    header = MP2::StringObject( objectType );
                    body = _( "You have taken control of the local Alchemist shop. It will provide you with %{count} unit of Mercury per day." );
                    break;

                case MP2::OBJ_SAWMILL:
                    resource = Resource::WOOD;
                    header = MP2::StringObject( objectType );
                    body = _( "You gain control of a sawmill. It will provide you with %{count} units of wood per day." );
                    break;

                case MP2::OBJ_MINE: {
                    resource = getDailyIncomeObjectResources( tile ).getFirstValidResource().first;
                    header = Maps::GetMineName( resource );

                    switch ( resource ) {
                    case Resource::ORE:
                        body = _( "You gain control of an ore mine. It will provide you with %{count} units of ore per day." );
                        break;
                    case Resource::SULFUR:
                        body = _( "You gain control of a sulfur mine. It will provide you with %{count} unit of sulfur per day." );
                        break;
                    case Resource::CRYSTAL:
                        body = _( "You gain control of a crystal mine. It will provide you with %{count} unit of crystal per day." );
                        break;
                    case Resource::GEMS:
                        body = _( "You gain control of a gem mine. It will provide you with %{count} unit of gems per day." );
                        break;
                    case Resource::GOLD:
                        body = _( "You gain control of a gold mine. It will provide you with %{count} gold per day." );
                        break;
                    default:
                        break;
                    }
                    break;
                }

                case MP2::OBJ_LIGHTHOUSE:
                    header = MP2::StringObject( objectType );
                    body = _( "The lighthouse is now under your control, and all of your ships will now move further each day." );
                    break;

                default:
                    body = _( "You gain control of a %{name}." );
                    header = MP2::StringObject( objectType );
                    StringReplace( body, "%{name}", MP2::StringObject( objectType ) );
                    break;
                }

                {
                    const MusicalEffectPlayer musicalEffectPlayer;

                    // The Lighthouse has a special sound
                    if ( objectType == MP2::OBJ_LIGHTHOUSE ) {
                        MusicalEffectPlayer::play( MUS::XANADU );
                    }

                    if ( resource == Resource::UNKNOWN ) {
                        fheroes2::showStandardTextMessage( std::move( header ), std::move( body ), Dialog::OK );
                    }
                    else {
                        DialogCaptureResourceObject( std::move( header ), std::move( body ), resource );
                    }
                }
            };

            if ( isCaptureObjectProtected( tile ) ) {
                Army army( tile );

                const Battle::Result result = Battle::Loader( hero.GetArmy(), army, dstIndex );

                if ( result.AttackerWins() ) {
                    hero.IncreaseExperience( result.GetExperienceAttacker() );

                    captureObject();
                }
                else {
                    // The army should include only the original monsters
                    const uint32_t monstersLeft = army.getTotalCount();
                    assert( monstersLeft == army.GetCountMonsters( getMonsterFromTile( tile ) ) );

                    Troop & troop = world.GetCapturedObject( dstIndex ).GetTroop();
                    troop.SetCount( monstersLeft );

                    // If all the guards are defeated, but the hero has lost the battle,
                    // just remove the protection from the object
                    if ( monstersLeft == 0 ) {
                        removeObjectProtection();
                    }

                    BattleLose( hero, result, true );
                }
            }
            else {
                captureObject();
            }
        }
    }

    void ActionToAbandonedMine( Heroes & hero, const MP2::MapObjectType objectType, const int32_t dstIndex )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        bool enter = false;

        {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::WATCHTOWER );

            enter = ( fheroes2::showStandardTextMessage( MP2::StringObject( objectType ),
                                                         _( "You come upon an abandoned gold mine. The mine appears to be haunted. Do you wish to enter?" ),
                                                         Dialog::YES | Dialog::NO )
                      == Dialog::YES );
        }

        if ( enter ) {
            Maps::Tiles & tile = world.GetTiles( dstIndex );

            Army army( tile );

            const Battle::Result result = Battle::Loader( hero.GetArmy(), army, dstIndex );

            if ( result.AttackerWins() ) {
                hero.IncreaseExperience( result.GetExperienceAttacker() );

                Maps::restoreAbandonedMine( tile, Resource::GOLD );
                hero.setObjectTypeUnderHero( MP2::OBJ_MINE );
                setColorOnTile( tile, hero.GetColor() );

                // TODO: make a function that will automatically get the object size in tiles and return a ROI for radar update.
                // Set the radar update ROI according to captured object size and position.
                const fheroes2::Point tilePoint = Maps::GetPoint( dstIndex );
                const fheroes2::Rect radarRoi( tilePoint.x - 1, tilePoint.y - 1, 3, 2 );

                Interface::AdventureMap & I = Interface::AdventureMap::Get();

                // Update the object on radar.
                I.getRadar().SetRenderArea( radarRoi );
                I.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

                fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), _( "You beat the Ghosts and are able to restore the mine to production." ),
                                                   Dialog::OK );
            }
            else {
                BattleLose( hero, result, true );
            }
        }
    }

    void ActionToDwellingJoinMonster( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Troop & troop = getTroopFromTile( tile );

        std::string title( MP2::StringObject( objectType ) );

        if ( troop.isValid() ) {
            std::string message = _( "A group of %{monster} with a desire for greater glory wish to join you. Do you accept?" );
            StringReplaceWithLowercase( message, "%{monster}", troop.GetMultiName() );

            bool recruit = false;

            {
                const MusicalEffectPlayer musicalEffectPlayer;

                // The Tree House has a special sound
                if ( objectType == MP2::OBJ_TREE_HOUSE && MusicalEffectPlayer::canBePlayed( MUS::TREEHOUSE ) ) {
                    MusicalEffectPlayer::play( MUS::TREEHOUSE );
                }
                else {
                    AudioManager::PlaySound( M82::EXPERNCE );
                }

                recruit = ( fheroes2::showStandardTextMessage( std::move( title ), std::move( message ), Dialog::YES | Dialog::NO ) == Dialog::YES );
            }

            if ( recruit ) {
                if ( !hero.GetArmy().CanJoinTroop( troop ) ) {
                    fheroes2::showStandardTextMessage( troop.GetName(), _( "You are unable to recruit at this time, your ranks are full." ), Dialog::OK );
                }
                else {
                    setMonsterCountOnTile( tile, 0 );
                    hero.GetArmy().JoinTroop( troop );

                    Interface::AdventureMap::Get().setRedraw( Interface::REDRAW_STATUS );
                }
            }
        }
        else {
            fheroes2::showStandardTextMessage( std::move( title ), _( "As you approach the dwelling, you notice that there is no one here." ), Dialog::OK );
        }

        hero.SetVisited( dst_index, Visit::GLOBAL );
    }

    void ActionToDwellingRecruitMonster( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        Maps::Tiles & tile = world.GetTiles( dst_index );

        std::string msg_full;
        std::string msg_void;

        switch ( objectType ) {
        case MP2::OBJ_RUINS:
            msg_void = _( "You search the ruins, but the Medusas that used to live here are gone. Perhaps there will be more next week." );
            msg_full = _( "You've found some Medusas living in the ruins. They are willing to join your army for a price. Do you want to recruit Medusas?" );
            break;

        case MP2::OBJ_TREE_CITY:
            msg_void = _( "You've found a Sprite Tree City. Unfortunately, none of the Sprites living there wish to join an army. Maybe next week." );
            msg_full = _( "Some of the Sprites living in the tree city are willing to join your army for a price. Do you want to recruit Sprites?" );
            break;

        case MP2::OBJ_WAGON_CAMP:
            msg_void = _( "A colorful Rogues' wagon stands empty here. Perhaps more Rogues will be here later." );
            msg_full = _( "Distant sounds of music and laughter draw you to a colorful wagon housing Rogues. Do you wish to have any Rogues join your army?" );
            break;

        case MP2::OBJ_DESERT_TENT:
            msg_void = _( "A group of tattered tents, billowing in the sandy wind, beckons you. The tents are unoccupied. Perhaps more Nomads will be here later." );
            msg_full = _( "A group of tattered tents, billowing in the sandy wind, beckons you. Do you wish to have any Nomads join you during your travels?" );
            break;

        case MP2::OBJ_EARTH_ALTAR:
            msg_void = _( "The pit of mud bubbles for a minute and then lies still." );
            msg_full = _(
                "As you approach the bubbling pit of mud, creatures begin to climb out and position themselves around it. In unison they say: \"Mother Earth would like to offer you a few of her troops. Do you want to recruit Earth Elementals?\"" );
            break;

        case MP2::OBJ_AIR_ALTAR:
            msg_void = _( "You enter the structure of white stone pillars, and find nothing." );
            msg_full = _(
                "White stone pillars support a roof that rises up to the sky. As you enter the structure, the dead air of the outside gives way to a whirling gust that almost pushes you back out. The air current materializes into a barely visible form. The creature asks, in what can only be described as a loud whisper: \"Why have you come? Are you here to call upon the forces of the air?\"" );
            break;

        case MP2::OBJ_FIRE_ALTAR:
            msg_void = _( "No Fire Elementals approach you from the lava pool." );
            msg_full = _(
                "Beneath a structure that serves to hold in heat, Fire Elementals move about in a fiery pool of molten lava. A group of them approach you and offer their services. Would you like to recruit Fire Elementals?" );
            break;

        case MP2::OBJ_WATER_ALTAR:
            msg_void = _( "A face forms in the water for a moment, and then is gone." );
            msg_full = _(
                "Crystalline structures cast shadows over a small reflective pool of water. You peer into the pool, and a face that is not your own peers back. It asks: \"Would you like to call upon the powers of water?\"" );
            break;

        case MP2::OBJ_BARROW_MOUNDS:
            msg_void = _( "This burial site is deathly still." );
            msg_full = _(
                "Restless spirits of long dead warriors seeking their final resting place offer to join you in hopes of finding peace. Do you wish to recruit ghosts?" );
            break;

        default:
            return;
        }

        const Troop & troop = getTroopFromTile( tile );

        const std::string title( MP2::StringObject( objectType ) );

        if ( !troop.isValid() ) {
            fheroes2::showStandardTextMessage( title, std::move( msg_void ), Dialog::OK );
        }
        else {
            bool recruit = false;

            {
                const MusicalEffectPlayer musicalEffectPlayer;

                // The Tree City and the Wagon Camp have a special sound
                if ( ( objectType == MP2::OBJ_TREE_CITY || objectType == MP2::OBJ_WAGON_CAMP ) && MusicalEffectPlayer::canBePlayed( MUS::TREEHOUSE ) ) {
                    MusicalEffectPlayer::play( MUS::TREEHOUSE );
                }
                // Changed OG selection to something more appropriate
                else if ( objectType == MP2::OBJ_DESERT_TENT && MusicalEffectPlayer::canBePlayed( MUS::ARABIAN ) ) {
                    MusicalEffectPlayer::play( MUS::ARABIAN );
                }
                else {
                    AudioManager::PlaySound( M82::EXPERNCE );
                }

                recruit = ( fheroes2::showStandardTextMessage( title, std::move( msg_full ), Dialog::YES | Dialog::NO ) == Dialog::YES );
            }

            if ( recruit ) {
                RecruitMonsterFromTile( hero, tile, title, troop, false );
            }
        }

        hero.SetVisited( dst_index, Visit::GLOBAL );
    }

    void ActionToDwellingBattleMonster( Heroes & hero, const MP2::MapObjectType objectType, const int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        const std::string title( MP2::StringObject( objectType ) );

        const char * objectIsEmptyMsg = nullptr;
        const char * recruitmentAvailableMsg = nullptr;
        const char * warningMsg = nullptr;
        const char * victoryMsg = nullptr;

        switch ( objectType ) {
        case MP2::OBJ_CITY_OF_DEAD:
            objectIsEmptyMsg = _( "The City of the Dead is empty of life, and empty of unlife as well. Perhaps some undead will move in next week." );
            recruitmentAvailableMsg = _( "Some Liches living here are willing to join your army for a price. Do you want to recruit Liches?" );
            warningMsg = _( "You've found the ruins of an ancient city, now inhabited solely by the undead. Will you search?" );
            victoryMsg
                = _( "Some of the surviving Liches are impressed by your victory over their fellows, and offer to join you for a price. Do you want to recruit Liches?" );
            break;
        case MP2::OBJ_TROLL_BRIDGE:
            objectIsEmptyMsg
                = _( "You've found one of those bridges that Trolls are so fond of living under, but there are none here. Perhaps there will be some next week." );
            recruitmentAvailableMsg = _( "Some Trolls living under a bridge are willing to join your army, but for a price. Do you want to recruit Trolls?" );
            warningMsg = _( "Trolls living under the bridge challenge you. Will you fight them?" );
            victoryMsg
                = _( "A few Trolls remain, cowering under the bridge. They approach you and offer to join your forces as mercenaries. Do you want to buy any Trolls?" );
            break;
        case MP2::OBJ_DRAGON_CITY:
            objectIsEmptyMsg = _( "The Dragon city has no Dragons willing to join you this week. Perhaps a Dragon will become available next week." );
            recruitmentAvailableMsg = _( "The Dragon city is willing to offer some Dragons for your army for a price. Do you wish to recruit Dragons?" );
            warningMsg
                = _( "You stand before the Dragon City, a place off-limits to mere humans. Do you wish to violate this rule and challenge the Dragons to a fight?" );
            victoryMsg
                = _( "Having defeated the Dragon champions, the city's leaders agree to supply some Dragons to your army for a price. Do you wish to recruit Dragons?" );
            break;
        default:
            assert( 0 );
            return;
        }

        enum class Outcome
        {
            Invalid,
            Empty,
            IgnoreRecruit,
            IgnoreFight,
            Recruit,
            Fight
        };

        const Outcome outcome = [dst_index, &title, objectIsEmptyMsg, recruitmentAvailableMsg, warningMsg]() {
            const Maps::Tiles & tile = world.GetTiles( dst_index );

            if ( getColorFromTile( tile ) != Color::NONE ) {
                const Troop troop = getTroopFromTile( tile );

                if ( !troop.isValid() ) {
                    fheroes2::showStandardTextMessage( title, objectIsEmptyMsg, Dialog::OK );

                    return Outcome::Empty;
                }

                const MusicalEffectPlayer musicalEffectPlayer( MUS::DUNGEON );

                if ( fheroes2::showStandardTextMessage( title, recruitmentAvailableMsg, Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                    return Outcome::Recruit;
                }

                return Outcome::IgnoreRecruit;
            }

            // In the original game, this dialog window has no sound, this is an improvement specific to fheroes2
            const MusicalEffectPlayer musicalEffectPlayer( MUS::DUNGEON );

            if ( fheroes2::showStandardTextMessage( title, warningMsg, Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                return Outcome::Fight;
            }

            return Outcome::IgnoreFight;
        }();

        Maps::Tiles & tile = world.GetTiles( dst_index );

        switch ( outcome ) {
        case Outcome::Empty:
        case Outcome::IgnoreRecruit:
            hero.SetVisited( dst_index, Visit::GLOBAL );

            break;
        case Outcome::IgnoreFight:
            break;
        case Outcome::Recruit: {
            const Troop troop = getTroopFromTile( tile );
            assert( troop.isValid() );

            RecruitMonsterFromTile( hero, tile, title, troop, false );

            hero.SetVisited( dst_index, Visit::GLOBAL );

            break;
        }
        case Outcome::Fight: {
            Army army( tile );

            const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );

                // Set ownership of the dwelling to a Neutral (gray) player so that any player can recruit troops without a fight.
                setColorOnTile( tile, Color::UNUSED );
                tile.SetObjectPassable( true );

                if ( fheroes2::showStandardTextMessage( title, victoryMsg, Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                    const Troop troop = getTroopFromTile( tile );
                    assert( troop.isValid() );

                    RecruitMonsterFromTile( hero, tile, title, troop, false );
                }

                hero.SetVisited( dst_index, Visit::GLOBAL );
            }
            else {
                BattleLose( hero, res, true );
            }

            break;
        }
        default:
            assert( 0 );
            break;
        }
    }

    void ActionToObservationTower( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::WATCHTOWER );

            fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), _( "From the observation tower, you are able to see distant lands." ), Dialog::OK );
        }

        const int32_t scoutRange = static_cast<int32_t>( GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ) );
        Maps::ClearFog( dst_index, scoutRange, hero.GetColor() );

        Interface::AdventureMap & I = Interface::AdventureMap::Get();
        const fheroes2::Point towerPosition = Maps::GetPoint( dst_index );
        const fheroes2::Rect towerRoi( towerPosition.x - scoutRange, towerPosition.y - scoutRange, 2 * scoutRange + 1, 2 * scoutRange + 1 );
        I.getRadar().SetRenderArea( towerRoi );
        I.setRedraw( Interface::REDRAW_RADAR );
    }

    void ActionToArtesianSpring( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        std::string title( MP2::StringObject( MP2::OBJ_ARTESIAN_SPRING ) );

        if ( world.isAnyKingdomVisited( objectType, dst_index ) ) {
            fheroes2::showStandardTextMessage( std::move( title ), _( "The spring only refills once a week, and someone's already been here this week." ), Dialog::OK );
        }
        else {
            const uint32_t max = hero.GetMaxSpellPoints();

            if ( hero.GetSpellPoints() >= max * 2 ) {
                const MusicalEffectPlayer musicalEffectPlayer( MUS::WATERSPRING );

                fheroes2::
                    showStandardTextMessage( std::move( title ),
                                             _( "A drink at the spring is supposed to give you twice your normal spell points, but you are already at that level." ),
                                             Dialog::OK );
            }
            else {
                hero.SetSpellPoints( max * 2 );

                {
                    const MusicalEffectPlayer musicalEffectPlayer( MUS::WATERSPRING );

                    fheroes2::showStandardTextMessage( std::move( title ),
                                                       _( "A drink from the spring fills your blood with magic! You have twice your normal spell points in reserve." ),
                                                       Dialog::OK );
                }
            }
        }

        hero.SetVisitedWideTile( dst_index, objectType, Visit::GLOBAL );
    }

    void ActionToXanadu( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string title( MP2::StringObject( objectType ) );

        if ( hero.isVisited( tile ) ) {
            fheroes2::showStandardTextMessage( std::move( title ),
                                               _( "Recognizing you, the butler refuses to admit you. \"The master,\" he says, \"will not see the same student twice.\"" ),
                                               Dialog::OK );
        }
        else {
            if ( GameStatic::isHeroWorthyToVisitXanadu( hero ) ) {
                {
                    const MusicalEffectPlayer musicalEffectPlayer( MUS::XANADU );

                    const fheroes2::SmallPrimarySkillDialogElement attackUI( Skill::Primary::ATTACK, "+1" );
                    const fheroes2::SmallPrimarySkillDialogElement defenseUI( Skill::Primary::DEFENSE, "+1" );
                    const fheroes2::SmallPrimarySkillDialogElement powerUI( Skill::Primary::POWER, "+1" );
                    const fheroes2::SmallPrimarySkillDialogElement knowledgeUI( Skill::Primary::KNOWLEDGE, "+1" );

                    fheroes2::showStandardTextMessage( std::move( title ),
                                                       _( "The butler admits you to see the master of the house. He trains you in the four skills a hero should know." ),
                                                       Dialog::OK, { &attackUI, &defenseUI, &powerUI, &knowledgeUI } );
                }

                hero.IncreasePrimarySkill( Skill::Primary::ATTACK );
                hero.IncreasePrimarySkill( Skill::Primary::DEFENSE );
                hero.IncreasePrimarySkill( Skill::Primary::KNOWLEDGE );
                hero.IncreasePrimarySkill( Skill::Primary::POWER );
                hero.SetVisited( dst_index );
            }
            else {
                fheroes2::showStandardTextMessage(
                    std::move( title ),
                    _( "The butler opens the door and looks you up and down. \"You are neither famous nor diplomatic enough to be admitted to see my master,\" he sniffs. \"Come back when you think yourself worthy.\"" ),
                    Dialog::OK );
            }
        }
    }

    bool ActionToUpgradeArmy( Army & army, const Monster & mons, std::string & str1, std::string & str2, const bool combineWithAnd )
    {
        const std::string combTypeAnd = _( " and " );
        const std::string combTypeComma = ", ";

        if ( army.HasMonster( mons ) ) {
            army.UpgradeMonsters( mons );
            if ( !str1.empty() )
                str1 += combineWithAnd ? combTypeAnd : combTypeComma;
            str1 += mons.GetMultiName();
            if ( !str2.empty() )
                str2 += combineWithAnd ? combTypeAnd : combTypeComma;
            str2 += mons.GetUpgrade().GetMultiName();
            return true;
        }
        return false;
    }

    void ActionToUpgradeArmyObject( Heroes & hero, const MP2::MapObjectType objectType, const std::string & defaultMessage )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )

        std::string monsters;
        std::string monsters_upgrade;
        std::string msg1;
        std::string msg2;

        std::vector<Monster *> mons;
        std::vector<Monster> monsToUpgrade;

        switch ( objectType ) {
        case MP2::OBJ_HILL_FORT: {
            monsToUpgrade = { Monster( Monster::OGRE ), Monster( Monster::ORC ), Monster( Monster::DWARF ) };

            msg1 = _( "All of the %{monsters} you have in your army have been trained by the battle masters of the fort. Your army now contains %{monsters2}." );
            msg2
                = _( "An unusual alliance of Ogres, Orcs, and Dwarves offer to train (upgrade) any such troops brought to them. Unfortunately, you have none with you." );
            break;
        }

        case MP2::OBJ_FREEMANS_FOUNDRY: {
            monsToUpgrade = { Monster( Monster::SWORDSMAN ), Monster( Monster::PIKEMAN ), Monster( Monster::IRON_GOLEM ) };

            msg1 = _( "All of your %{monsters} have been upgraded into %{monsters2}." );
            msg2 = _(
                "A blacksmith working at the foundry offers to convert all Pikemen and Swordsmen's weapons brought to him from iron to steel. He also says that he knows a process that will convert Iron Golems into Steel Golems. Unfortunately, you have none of these troops in your army, so he can't help you." );
            break;
        }

        case MP2::OBJ_STABLES: {
            assert( !defaultMessage.empty() );
            msg1 = defaultMessage;
            msg2 = defaultMessage;

            monsToUpgrade = { Monster( Monster::CAVALRY ) };
            break;
        }

        default:
            ERROR_LOG( "Incorrect object type passed to ActionToUpgradeArmyObject" )
            assert( 0 );
            return;
        }

        if ( monsToUpgrade.empty() ) {
            ERROR_LOG( "monsToUpgrade mustn't be empty." )
            assert( 0 );
            return;
        }

        Army & heroArmy = hero.GetArmy();
        mons.reserve( monsToUpgrade.size() );

        for ( size_t i = 0; i < monsToUpgrade.size(); ++i ) {
            if ( !heroArmy.HasMonster( monsToUpgrade[i] ) )
                continue;
            const bool combineWithAnd = i == ( monsToUpgrade.size() - 1 ) && !mons.empty();
            if ( ActionToUpgradeArmy( heroArmy, monsToUpgrade[i], monsters, monsters_upgrade, combineWithAnd ) )
                mons.emplace_back( &monsToUpgrade[i] );
        }

        std::string title( MP2::StringObject( objectType ) );

        if ( !mons.empty() ) {
            // composite sprite
            int32_t offsetX = 0;
            int32_t offsetY = 0;
            const fheroes2::Sprite & border = fheroes2::AGG::GetICN( ICN::STRIP, 12 );

            const int32_t monsterCount = static_cast<int32_t>( mons.size() ); // safe to do as the count is no more than 3

            const int32_t monsterPerX = monsterCount > 1 ? 2 : 1;
            const int32_t monsterPerY = monsterCount == 1 ? 1 : ( monsterCount + ( monsterCount - 1 ) ) / 2;

            fheroes2::Image surface( border.width() * monsterPerX + ( monsterPerX - 1 ) * 4, border.height() * monsterPerY + ( monsterPerY - 1 ) * 4 );
            surface.reset();

            StringReplace( msg1, "%{monsters}", monsters );
            StringReplace( msg1, "%{monsters2}", monsters_upgrade );

            for ( size_t i = 0; i < mons.size(); ++i ) {
                if ( i > 0 && ( i & 1 ) == 0 ) {
                    if ( i == mons.size() - 1 ) {
                        offsetX = border.width() / 2 + 2;
                    }
                    else {
                        offsetX = 0;
                    }
                    offsetY += border.height() + 4;
                }

                // border
                fheroes2::Blit( border, surface, offsetX, offsetY );
                fheroes2::renderMonsterFrame( mons[i]->GetUpgrade(), surface, { 6 + offsetX, 6 + offsetY } );

                offsetX += border.width() + 4;
            }

            {
                const MusicalEffectPlayer musicalEffectPlayer;

                // The Hill Fort has a special sound
                if ( objectType == MP2::OBJ_HILL_FORT ) {
                    MusicalEffectPlayer::play( MUS::HILLFORT );
                }

                const fheroes2::CustomImageDialogElement imageUI( std::move( surface ) );
                fheroes2::showStandardTextMessage( std::move( title ), std::move( msg1 ), Dialog::OK, { &imageUI } );
            }
        }
        else {
            fheroes2::showStandardTextMessage( std::move( title ), std::move( msg2 ), Dialog::OK );
        }
    }

    void ActionToMagellanMaps( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const Funds payment = PaymentConditions::getMagellansMapsPurchasePrice();
        Kingdom & kingdom = hero.GetKingdom();

        std::string title( MP2::StringObject( objectType ) );

        if ( hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) ) {
            fheroes2::showStandardTextMessage(
                std::move( title ), _( "The captain looks at you with surprise and says:\n\"You already have all the maps I know about. Let me fish in peace now.\"" ),
                Dialog::OK );
        }
        else {
            if ( kingdom.AllowPayment( payment ) ) {
                bool buy = false;

                {
                    const MusicalEffectPlayer musicalEffectPlayer( MUS::WATCHTOWER );

                    buy = ( fheroes2::showStandardTextMessage(
                                std::move( title ),
                                _( "A retired captain living on this refurbished fishing platform offers to sell you maps of the sea he made in his younger days for 1,000 gold. Do you wish to buy the maps?" ),
                                Dialog::YES | Dialog::NO )
                            == Dialog::YES );
                }

                if ( buy ) {
                    world.ActionForMagellanMaps( hero.GetColor() );

                    kingdom.OddFundsResource( payment );

                    hero.SetVisited( dst_index, Visit::GLOBAL );
                    hero.setVisitedForAllies( dst_index );

                    // Fully update fog directions and redraw radar and game area.
                    Interface::GameArea::updateMapFogDirections();
                    Interface::AdventureMap & I = Interface::AdventureMap::Get();
                    I.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );
                }
            }
            else {
                const MusicalEffectPlayer musicalEffectPlayer( MUS::WATCHTOWER );

                fheroes2::showStandardTextMessage( std::move( title ),
                                                   _( "The captain sighs. \"You don't have enough money, eh? You can't expect me to give my maps away for free!\"" ),
                                                   Dialog::OK );
            }
        }
    }

    void ActionToEvent( Heroes & hero, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        MapEvent * event_maps = world.GetMapEvent( Maps::GetPoint( dst_index ) );

        if ( event_maps && event_maps->isAllow( hero.GetColor() ) ) {
            hero.SetMove( false );

            const Funds fundsToUpdate = Resource::CalculateEventResourceUpdate( hero.GetKingdom().GetFunds(), event_maps->resources );

            if ( event_maps->resources.GetValidItemsCount() ) {
                hero.GetKingdom().AddFundsResource( event_maps->resources );
            }

            const Artifact & art = event_maps->artifact;
            const bool hasValidArtifact = art.isValid();

            const std::vector<fheroes2::ResourceDialogElement> resourceUI = fheroes2::getResourceDialogElements( fundsToUpdate );

            std::vector<const fheroes2::DialogElement *> elementUI;
            elementUI.reserve( resourceUI.size() + ( hasValidArtifact ? 1 : 0 ) );

            for ( const fheroes2::ResourceDialogElement & element : resourceUI ) {
                elementUI.emplace_back( &element );
            }

            // Check for the presence of an artifact as a reward in the event and display it in the dialog.
            std::unique_ptr<fheroes2::ArtifactDialogElement> artifactUI;
            if ( hasValidArtifact ) {
                artifactUI = std::make_unique<fheroes2::ArtifactDialogElement>( art );
                AudioManager::PlaySound( M82::TREASURE );
                elementUI.emplace_back( artifactUI.get() );
            }

            fheroes2::showStandardTextMessage( {}, event_maps->message, Dialog::OK, elementUI );

            // PickupArtifact() has a built-in check for Artifact correctness, the presence of a magic book
            // and the fullness of the bag. Is also displays appropriate text when an artifact cannot be picked up.
            hero.PickupArtifact( art );

            event_maps->SetVisited();

            if ( event_maps->isSingleTimeEvent ) {
                hero.setObjectTypeUnderHero( MP2::OBJ_NONE );
                world.RemoveMapObject( event_maps );
            }
        }
    }

    void ActionToObelisk( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        Kingdom & kingdom = hero.GetKingdom();
        std::string title( MP2::StringObject( objectType ) );

        if ( !hero.isVisited( world.GetTiles( dst_index ), Visit::GLOBAL ) ) {
            hero.SetVisited( dst_index, Visit::GLOBAL );
            kingdom.PuzzleMaps().Update( kingdom.CountVisitedObjects( MP2::OBJ_OBELISK ), world.CountObeliskOnMaps() );
            AudioManager::PlaySound( M82::EXPERNCE );
            fheroes2::showStandardTextMessage(
                std::move( title ),
                _( "You come upon an obelisk made from a type of stone you have never seen before. Staring at it intensely, the smooth surface suddenly changes to an inscription. The inscription is a piece of a lost ancient map. Quickly you copy down the piece and the inscription vanishes as abruptly as it appeared." ),
                Dialog::OK );
            kingdom.PuzzleMaps().ShowMapsDialog();
        }
        else {
            fheroes2::showStandardTextMessage( std::move( title ), _( "You have already been to this obelisk." ), Dialog::OK );
        }
    }

    void ActionToTreeKnowledge( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const Maps::Tiles & tile = world.GetTiles( dst_index );
        std::string title( MP2::StringObject( objectType ) );

        if ( hero.isVisited( tile ) ) {
            fheroes2::showStandardTextMessage(
                std::move( title ),
                _( "Upon your approach, the tree opens its eyes in delight. \"It is good to see you, my student. I hope my teachings have helped you.\"" ), Dialog::OK );
        }
        else {
            const Funds & payment = getTreeOfKnowledgeRequirement( tile );
            bool increaseExperience = ( payment.GetValidItemsCount() == 0 );

            const int level = hero.GetLevel();
            assert( level > 0 );
            const uint32_t possibleExperience = Heroes::GetExperienceFromLevel( level ) - Heroes::GetExperienceFromLevel( level - 1 );

            {
                const MusicalEffectPlayer musicalEffectPlayer( MUS::EXPERIENCE );

                // Free training
                if ( increaseExperience ) {
                    const fheroes2::ExperienceDialogElement experienceUI( static_cast<int32_t>( possibleExperience ) );

                    // In the original game, there was no way to refuse to level up for free, this is an improvement specific to fheroes2
                    increaseExperience
                        = ( fheroes2::showStandardTextMessage(
                                std::move( title ),
                                _( "Upon your approach, the tree opens its eyes in delight. \"Ahh, an adventurer! Allow me to teach you a little of what I have learned over the ages.\"" ),
                                Dialog::YES | Dialog::NO, { &experienceUI } )
                            == Dialog::YES );
                }
                else {
                    const auto rc = payment.getFirstValidResource();

                    if ( hero.GetKingdom().AllowPayment( payment ) ) {
                        std::string msg = _( "Upon your approach, the tree opens its eyes in delight." );
                        msg += '\n';
                        msg.append(
                            _( "\"Ahh, an adventurer! I will be happy to teach you a little of what I have learned over the ages for a mere %{count} %{res}.\"" ) );
                        msg += '\n';
                        msg.append( _( "(Just bury it around my roots.)" ) );

                        StringReplace( msg, "%{res}", Resource::String( rc.first ) );
                        StringReplace( msg, "%{count}", std::to_string( rc.second ) );

                        const fheroes2::ExperienceDialogElement experienceUI( static_cast<int32_t>( possibleExperience ) );

                        increaseExperience
                            = ( fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::YES | Dialog::NO, { &experienceUI } ) == Dialog::YES );
                    }
                    else {
                        std::string msg = _( "Tears brim in the eyes of the tree." );
                        msg += '\n';
                        msg.append( _( "\"I need %{count} %{res}.\"" ) );
                        msg += '\n';
                        msg.append( _( "it whispers. (sniff) \"Well, come back when you can pay me.\"" ) );
                        StringReplace( msg, "%{res}", Resource::String( rc.first ) );
                        StringReplace( msg, "%{count}", std::to_string( rc.second ) );

                        fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
                    }
                }
                hero.SetVisited( dst_index, Visit::GLOBAL );
            }

            if ( increaseExperience ) {
                hero.GetKingdom().OddFundsResource( payment );
                hero.SetVisited( dst_index );
                hero.IncreaseExperience( possibleExperience );
            }
        }
    }

    void ActionToOracle( const Heroes & hero, const MP2::MapObjectType objectType )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )
#ifndef WITH_DEBUG
        (void)hero;
#endif

        {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::WATERSPRING );

            fheroes2::showStandardTextMessage(
                MP2::StringObject( objectType ),
                _( "Nestled among the trees sits a blind seer. After you explain the intent of your journey, the seer activates his crystal ball, allowing you to see the strengths and weaknesses of your opponents." ),
                Dialog::OK );
        }

        Dialog::ThievesGuild( true );
    }

    void ActionToDaemonCave( Heroes & hero, const MP2::MapObjectType objectType, const int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const std::string title = MP2::StringObject( objectType );

        enum class Outcome
        {
            Invalid,
            Ignore,
            Empty,
            BattleWithServants,
            Experience,
            ExperienceAndGold,
            ExperienceAndArtifact,
            PayOff,
            Death
        };

        const uint32_t demonSlayingExperience = 1000;

        // Implicitly capture everything by reference because different compilers disagree on whether there is a need to capture the 'demonSlayingExperience' or not
        const Outcome outcome = [&]() {
            const MusicalEffectPlayer musicalEffectPlayer( MUS::DEMONCAVE );

            const Maps::Tiles & tile = world.GetTiles( dst_index );

            if ( fheroes2::showStandardTextMessage( title,
                                                    _( "The entrance to the cave is dark, and a foul, sulfurous smell issues from the cave mouth. Will you enter?" ),
                                                    Dialog::YES | Dialog::NO )
                 != Dialog::YES ) {
                return Outcome::Ignore;
            }

            if ( !doesTileContainValuableItems( tile ) ) {
                fheroes2::showStandardTextMessage( title, _( "Except for evidence of a terrible battle, the cave is empty." ), Dialog::OK );

                return Outcome::Empty;
            }

            if (
                fheroes2::showStandardTextMessage(
                    title,
                    _( "You find a powerful and grotesque Demon in the cave. \"Today,\" it rasps, \"you will fight and surely die. But I will give you a choice of deaths. You may fight me, or you may fight my servants. Do you prefer to fight my servants?\"" ),
                    Dialog::YES | Dialog::NO )
                == Dialog::YES ) {
                return Outcome::BattleWithServants;
            }

            const Maps::DaemonCaveCaptureBonus originalDaemonBonus = getDaemonCaveBonusType( tile );

            const Maps::DaemonCaveCaptureBonus bonus
                = ( originalDaemonBonus == Maps::DaemonCaveCaptureBonus::GET_1000_EXPERIENCE_AND_ARTIFACT && hero.IsFullBagArtifacts() )
                      ? Maps::DaemonCaveCaptureBonus::GET_1000_EXPERIENCE_AND_2500_GOLD
                      : originalDaemonBonus;

            switch ( bonus ) {
            case Maps::DaemonCaveCaptureBonus::GET_1000_EXPERIENCE: {
                std::string msg
                    = _( "The Demon screams its challenge and attacks! After a short, desperate battle, you slay the monster and receive %{exp} experience points." );
                StringReplace( msg, "%{exp}", std::to_string( demonSlayingExperience ) );

                const fheroes2::ExperienceDialogElement experienceUI( demonSlayingExperience );
                fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::OK, { &experienceUI } );

                return Outcome::Experience;
            }
            case Maps::DaemonCaveCaptureBonus::GET_1000_EXPERIENCE_AND_2500_GOLD: {
                const Funds funds = getFundsFromTile( tile );
                assert( funds.gold > 0 || funds.GetValidItemsCount() == 0 );

                const uint32_t gold = funds.gold;

                std::string msg = _(
                    "The Demon screams its challenge and attacks! After a short, desperate battle, you slay the monster and receive %{exp} experience points and %{count} gold." );
                StringReplace( msg, "%{exp}", std::to_string( demonSlayingExperience ) );
                StringReplace( msg, "%{count}", std::to_string( gold ) );

                const fheroes2::ExperienceDialogElement experienceUI( demonSlayingExperience );
                const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );
                fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::OK, { &experienceUI, &goldUI } );

                return Outcome::ExperienceAndGold;
            }
            case Maps::DaemonCaveCaptureBonus::GET_1000_EXPERIENCE_AND_ARTIFACT: {
                const Artifact art = getArtifactFromTile( tile );
                if ( !art.isValid() ) {
                    return Outcome::Invalid;
                }

                std::string msg = _(
                    "The Demon screams its challenge and attacks! After a short, desperate battle, you slay the monster and find the %{art} in the back of the cave." );
                StringReplace( msg, "%{art}", art.GetName() );

                const fheroes2::ExperienceDialogElement experienceUI( demonSlayingExperience );
                const fheroes2::ArtifactDialogElement artifactUI( art );
                fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::OK, { &experienceUI, &artifactUI } );

                return Outcome::ExperienceAndArtifact;
            }
            case Maps::DaemonCaveCaptureBonus::PAY_2500_GOLD: {
                const Kingdom & kingdom = hero.GetKingdom();
                const Funds payment( getDaemonPaymentCondition( tile ) );

                if ( !kingdom.AllowPayment( payment ) ) {
                    std::string msg = _( "Seeing that you do not have %{count} gold, the demon slashes you with its claws, and the last thing you see is a red haze." );
                    StringReplace( msg, "%{count}", std::to_string( payment.gold ) );

                    fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::OK );

                    return Outcome::Death;
                }

                std::string msg = _(
                    "The Demon leaps upon you and has its claws at your throat before you can even draw your sword. \"Your life is mine,\" it says. \"I will sell it back to you for %{count} gold.\"" );
                StringReplace( msg, "%{count}", std::to_string( payment.gold ) );

                if ( fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::YES | Dialog::NO ) == Dialog::YES ) {
                    return Outcome::PayOff;
                }

                return Outcome::Death;
            }
            case Maps::DaemonCaveCaptureBonus::EMPTY:
                break;
            default:
                // This condition should never happen.
                assert( 0 );
                break;
            }

            return Outcome::Invalid;
        }();

        if ( outcome != Outcome::Ignore ) {
            Kingdom & kingdom = hero.GetKingdom();

            if ( outcome != Outcome::Empty ) {
                Maps::Tiles & tile = world.GetTiles( dst_index );
                assert( doesTileContainValuableItems( tile ) );

                switch ( outcome ) {
                case Outcome::BattleWithServants: {
                    Army army( tile );

                    const Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
                    if ( res.AttackerWins() ) {
                        hero.IncreaseExperience( res.GetExperienceAttacker() );

                        // Daemon Cave always gives 2500 Gold after a battle.
                        const uint32_t gold = 2500;

                        std::string msg = _( "Upon defeating the daemon's servants, you find a hidden cache with %{count} gold." );
                        StringReplace( msg, "%{count}", gold );

                        const fheroes2::ResourceDialogElement goldUI( Resource::GOLD, std::to_string( gold ) );

                        fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::OK, { &goldUI } );

                        kingdom.AddFundsResource( Funds( Resource::GOLD, gold ) );
                    }
                    else {
                        BattleLose( hero, res, true );
                    }

                    break;
                }
                case Outcome::Experience:
                    hero.IncreaseExperience( demonSlayingExperience );

                    break;
                case Outcome::ExperienceAndGold: {
                    hero.IncreaseExperience( demonSlayingExperience );
                    kingdom.AddFundsResource( getFundsFromTile( tile ) );

                    break;
                }
                case Outcome::ExperienceAndArtifact: {
                    const Artifact art = getArtifactFromTile( tile );

                    hero.IncreaseExperience( demonSlayingExperience );
                    hero.PickupArtifact( art );

                    break;
                }
                case Outcome::PayOff: {
                    kingdom.OddFundsResource( getDaemonPaymentCondition( tile ) );

                    break;
                }
                case Outcome::Death: {
                    Battle::Result res;
                    res.army1 = Battle::RESULT_LOSS;

                    BattleLose( hero, res, true );

                    break;
                }
                default:
                    assert( 0 );
                    break;
                }

                resetObjectMetadata( tile );
            }

            // Even if the hero has been defeated by a demon (and no longer belongs to any
            // valid kingdom), this tile should be marked as visited for his former kingdom.
            kingdom.SetVisited( dst_index, objectType );
        }
    }

    void ActionToAlchemistTower( Heroes & hero )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        BagArtifacts & bag = hero.GetBagArtifacts();
        const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.containsCurses(); } ) );

        std::string title = MP2::StringObject( MP2::OBJ_ALCHEMIST_TOWER );

        if ( cursed ) {
            const Funds payment = PaymentConditions::ForAlchemist();

            std::string msg = _( "As you enter the Alchemist's Tower, a hobbled, graying man in a brown cloak makes his way towards you." );
            msg += '\n';
            msg.append(
                _n( "He checks your pack, and sees that you have 1 cursed item.", "He checks your pack, and sees that you have %{count} cursed items.", cursed ) );
            StringReplace( msg, "%{count}", cursed );
            msg += '\n';
            msg.append( _n( "For %{gold} gold, the alchemist will remove it for you. Do you pay?",
                            "For %{gold} gold, the alchemist will remove them for you. Do you pay?", cursed ) );
            StringReplace( msg, "%{gold}", payment.gold );

            AudioManager::PlaySound( M82::EXPERNCE );

            if ( Dialog::YES == fheroes2::showStandardTextMessage( title, std::move( msg ), Dialog::YES | Dialog::NO ) ) {
                if ( hero.GetKingdom().AllowPayment( payment ) ) {
                    hero.GetKingdom().OddFundsResource( payment );

                    for ( Artifact & artifact : bag ) {
                        if ( artifact.containsCurses() ) {
                            artifact = Artifact::UNKNOWN;
                        }
                    }

                    msg = _n(
                        "After you consent to pay the requested amount of gold, the alchemist grabs the cursed artifact and throws it into his magical cauldron.",
                        "After you consent to pay the requested amount of gold, the alchemist grabs all cursed artifacts and throws them into his magical cauldron.",
                        cursed );

                    AudioManager::PlaySound( M82::GOODLUCK );

                    fheroes2::showStandardTextMessage( std::move( title ), std::move( msg ), Dialog::OK );
                }
                else {
                    fheroes2::showStandardTextMessage( std::move( title ),
                                                       _( "You hear a voice from behind the locked door, \"You don't have enough gold to pay for my services.\"" ),
                                                       Dialog::OK );
                }
            }
        }
        else {
            fheroes2::showStandardTextMessage( std::move( title ), _( "You hear a voice from high above in the tower, \"Go away! I can't help you!\"" ), Dialog::OK );
        }
    }

    void ActionToStables( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const bool isCavalryPresent = hero.GetArmy().HasMonster( Monster::CAVALRY );
        const bool visited = hero.isObjectTypeVisited( objectType );
        std::string body;

        if ( isCavalryPresent && visited ) {
            body = _(
                "The head groom speaks to you, \"That is a fine looking horse you have. I am afraid we can give you no better, but the horses your cavalry are riding look to be of poor breeding stock. We have many trained war horses which would aid your riders greatly. I insist you take them.\"" );
        }
        else if ( isCavalryPresent && !visited ) {
            body = _(
                "As you approach the stables, the head groom appears, leading a fine looking war horse. \"This steed will help speed you in your travels. Alas, he will grow tired in a week. You must also let me give better horses to your mounted soldiers, their horses look shoddy and weak.\"" );
        }
        else if ( !isCavalryPresent && visited ) {
            body = _(
                "The head groom approaches you and speaks, \"You already have a fine horse, and have no inexperienced cavalry which might make use of our trained war horses.\"" );
        }
        else {
            body = _(
                "As you approach the stables, the head groom appears, leading a fine looking war horse. \"This steed will help speed you in your travels. Alas, his endurance will wane with a lot of heavy riding, and you must return for a fresh mount in a week. We also have many fine war horses which could benefit mounted soldiers, but you have none we can help.\"" );
        }

        if ( !visited ) {
            hero.SetVisited( dst_index );
            AudioManager::PlaySound( M82::EXPERNCE );
            hero.IncreaseMovePoints( GameStatic::getMovementPointBonus( MP2::OBJ_STABLES ) );
        }

        if ( isCavalryPresent ) {
            ActionToUpgradeArmyObject( hero, objectType, body );
        }
        else {
            fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), std::move( body ), Dialog::OK );
        }
    }

    void ActionToArena( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        if ( hero.isObjectTypeVisited( objectType ) ) {
            fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), _( "The Arena guards turn you away." ), Dialog::OK );
        }
        else {
            hero.SetVisited( dst_index );
            AudioManager::PlaySound( M82::EXPERNCE );
            hero.IncreasePrimarySkill( Dialog::SelectSkillFromArena() );
        }
    }

    void ActionToSirens( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        std::string title( MP2::StringObject( objectType ) );

        if ( hero.isObjectTypeVisited( objectType ) ) {
            fheroes2::showStandardTextMessage(
                std::move( title ), _( "You have your crew stop up their ears with wax before the sirens' eerie song has any chance of luring them to a watery grave." ),
                Dialog::OK );
        }
        else {
            const uint32_t experience = hero.GetArmy().ActionToSirens();
            if ( experience == 0 ) {
                fheroes2::showStandardTextMessage(
                    std::move( title ),
                    _( "As the sirens sing their eerie song, your small, determined army manages to overcome the urge to dive headlong into the sea." ), Dialog::OK );
            }
            else {
                const fheroes2::ExperienceDialogElement experienceUI( static_cast<int32_t>( experience ) );

                std::string str = _(
                    "An eerie wailing song emanates from the sirens perched upon the rocks. Many of your crew fall under its spell, and dive into the water where they drown. You are now wiser for the visit, and gain %{exp} experience." );
                StringReplace( str, "%{exp}", experience );

                AudioManager::PlaySound( M82::EXPERNCE );

                fheroes2::showStandardTextMessage( std::move( title ), std::move( str ), Dialog::OK, { &experienceUI } );

                hero.IncreaseExperience( experience );
            }

            hero.SetVisited( dst_index );
        }
    }

    void ActionToJail( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        const Kingdom & kingdom = hero.GetKingdom();
        std::string title( MP2::StringObject( objectType ) );

        if ( kingdom.AllowRecruitHero( false ) ) {
            const Maps::Tiles & tile = world.GetTiles( dst_index );
            AudioManager::PlaySound( M82::EXPERNCE );
            fheroes2::showStandardTextMessage(
                std::move( title ),
                _( "In a dazzling display of daring, you break into the local jail and free the hero imprisoned there, who, in return, pledges loyalty to your cause." ),
                Dialog::OK );

            runActionObjectFadeOutAnumation( tile, objectType );

            // TODO: add hero fading in animation together with jail animation.
            Heroes * prisoner = world.FromJailHeroes( dst_index );

            if ( prisoner ) {
                prisoner->Recruit( hero.GetColor(), Maps::GetPoint( dst_index ) );

                // Update the kingdom heroes list including the scrollbar.
                Interface::AdventureMap::Get().GetIconsPanel().ResetIcons( ICON_HEROES );
            }
        }
        else {
            std::string str = _( "You already have %{count} heroes, and regretfully must leave the prisoner in this jail to languish in agony for untold days." );
            StringReplace( str, "%{count}", Kingdom::GetMaxHeroes() );
            fheroes2::showStandardTextMessage( std::move( title ), std::move( str ), Dialog::OK );
        }
    }

    void ActionToHutMagi( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        AudioManager::PlaySound( M82::EXPERNCE );

        fheroes2::showStandardTextMessage(
            MP2::StringObject( objectType ),
            _( "You enter a rickety hut and talk to the magician who lives there. He tells you of places near and far which may aid you in your journeys." ),
            Dialog::OK );

        if ( !hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) ) {
            hero.SetVisited( dst_index, Visit::GLOBAL );

            const MapsIndexes eyeMagiIndexes = Maps::GetObjectPositions( MP2::OBJ_EYE_OF_MAGI );
            if ( !eyeMagiIndexes.empty() ) {
                Interface::AdventureMap & I = Interface::AdventureMap::Get();

                fheroes2::Display & display = fheroes2::Display::instance();

                const size_t maxDelay = 7;

                const int32_t scoutRange = static_cast<int32_t>( GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::MAGI_EYES ) );
                bool skipAnimation = false;
                fheroes2::Rect radarRenderArea;

                for ( const int32_t eyeIndex : eyeMagiIndexes ) {
                    Maps::ClearFog( eyeIndex, scoutRange, hero.GetColor() );

                    const fheroes2::Point eyePosition = Maps::GetPoint( eyeIndex );
                    const fheroes2::Rect eyeRoi( eyePosition.x - scoutRange, eyePosition.y - scoutRange, 2 * scoutRange + 1, 2 * scoutRange + 1 );

                    if ( skipAnimation ) {
                        radarRenderArea = fheroes2::getBoundaryRect( radarRenderArea, eyeRoi );
                        continue;
                    }

                    I.getGameArea().SetCenter( eyePosition );

                    I.getRadar().SetRenderArea( eyeRoi );
                    I.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

                    display.render();

                    LocalEvent & le = LocalEvent::Get();
                    size_t delay = 0;

                    while ( delay < maxDelay && le.HandleEvents( Game::isDelayNeeded( { Game::MAPS_DELAY } ) ) ) {
                        if ( le.isAnyKeyPressed() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() ) {
                            skipAnimation = true;
                            break;
                        }

                        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                            ++delay;
                            Game::updateAdventureMapAnimationIndex();
                            I.redraw( Interface::REDRAW_GAMEAREA );

                            display.render();
                        }
                    }
                }

                if ( skipAnimation ) {
                    I.getRadar().SetRenderArea( radarRenderArea );
                    I.setRedraw( Interface::REDRAW_RADAR );
                }

                I.getGameArea().SetCenter( hero.GetCenter() );
                I.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR_CURSOR );

                display.render();
            }
        }
    }

    void ActionToEyeMagi( const Heroes & hero, const MP2::MapObjectType objectType )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )
#ifndef WITH_DEBUG
        (void)hero;
#endif

        fheroes2::showStandardTextMessage( MP2::StringObject( objectType ), _( "This eye seems to be intently studying its surroundings." ), Dialog::OK );
    }

    void ActionToSphinx( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        enum class Outcome
        {
            Invalid,
            Empty,
            Ignore,
            CorrectAnswer,
            IncorrectAnswer
        };

        MapSphinx * riddle = dynamic_cast<MapSphinx *>( world.GetMapObject( dst_index ) );

        const Outcome outcome = [objectType, riddle]() {
            const std::string title = MP2::StringObject( objectType );
            const MusicalEffectPlayer musicalEffectPlayer( MUS::ARABIAN );

            if ( riddle == nullptr || !riddle->valid ) {
                fheroes2::showStandardTextMessage( title, _( "You come across a giant Sphinx. The Sphinx remains strangely quiet." ), Dialog::OK );

                return Outcome::Empty;
            }

            if (
                fheroes2::showStandardTextMessage(
                    title,
                    _( "\"I have a riddle for you,\" the Sphinx says. \"Answer correctly, and you shall be rewarded. Answer incorrectly, and you shall be eaten. Do you accept the challenge?\"" ),
                    Dialog::YES | Dialog::NO )
                != Dialog::YES ) {
                return Outcome::Ignore;
            }

            std::string question( _( "The Sphinx asks you the following riddle:\n\n'%{riddle}'\n\nYour answer?" ) );
            StringReplace( question, "%{riddle}", riddle->riddle );

            std::string answer;
            Dialog::inputString( question, answer, title, 0, false, false );

            if ( !riddle->isCorrectAnswer( answer ) ) {
                fheroes2::showStandardTextMessage(
                    title,
                    _( "\"You guessed incorrectly,\" the Sphinx says, smiling. The Sphinx swipes at you with a paw, knocking you to the ground. Another blow makes the world go black, and you know no more." ),
                    Dialog::OK );

                return Outcome::IncorrectAnswer;
            }

            const Funds & res = riddle->resources;
            const Artifact & art = riddle->artifact;
            const uint32_t count = res.GetValidItemsCount();

            if ( count || art.isValid() ) {
                const std::vector<fheroes2::ResourceDialogElement> resourceUiElements = fheroes2::getResourceDialogElements( res );

                std::vector<const fheroes2::DialogElement *> uiElements;
                uiElements.reserve( resourceUiElements.size() + 1 );

                for ( const fheroes2::ResourceDialogElement & element : resourceUiElements ) {
                    uiElements.emplace_back( &element );
                }

                std::unique_ptr<fheroes2::ArtifactDialogElement> artifactUI;

                if ( art.isValid() ) {
                    artifactUI = std::make_unique<fheroes2::ArtifactDialogElement>( art );

                    uiElements.emplace_back( artifactUI.get() );
                }

                fheroes2::
                    showStandardTextMessage( title,
                                             _( "Looking somewhat disappointed, the Sphinx sighs. \"You've answered my riddle so here's your reward. Now begone.\"" ),
                                             Dialog::OK, uiElements );

                return Outcome::CorrectAnswer;
            }

            return Outcome::Invalid;
        }();

        switch ( outcome ) {
        case Outcome::Empty:
            hero.SetVisited( dst_index, Visit::GLOBAL );

            break;
        case Outcome::Ignore:
            break;
        case Outcome::CorrectAnswer: {
            assert( riddle && riddle->valid );

            const Funds & res = riddle->resources;
            const Artifact & art = riddle->artifact;
            const uint32_t count = res.GetValidItemsCount();

            if ( count ) {
                hero.GetKingdom().AddFundsResource( res );
            }

            if ( art.isValid() ) {
                hero.PickupArtifact( art );
            }

            riddle->reset();

            hero.SetVisited( dst_index, Visit::GLOBAL );

            break;
        }
        case Outcome::IncorrectAnswer: {
            Battle::Result result;
            result.army1 = Battle::RESULT_LOSS;

            BattleLose( hero, result, true );

            break;
        }
        default:
            assert( 0 );
            break;
        }
    }

    void ActionToBarrier( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        // A hero cannot stand on a barrier. He must stand in front of the barrier. Something wrong with logic!
        assert( hero.GetIndex() != dst_index );

        const Maps::Tiles & tile = world.GetTiles( dst_index );
        const Kingdom & kingdom = hero.GetKingdom();

        std::string title = MP2::StringObject( objectType );

        if ( kingdom.IsVisitTravelersTent( getColorFromTile( tile ) ) ) {
            AudioManager::PlaySound( M82::EXPERNCE );

            fheroes2::showStandardTextMessage(
                std::move( title ),
                _( "A magical barrier stands tall before you, blocking your way. Runes on the arch read,\n\"Speak the key and you may pass.\"\nAs you speak the magic word, the glowing barrier dissolves into nothingness." ),
                Dialog::OK );

            AudioManager::PlaySound( M82::KILLFADE );

            runActionObjectFadeOutAnumation( tile, objectType );
        }
        else {
            fheroes2::showStandardTextMessage(
                std::move( title ),
                _( "A magical barrier stands tall before you, blocking your way. Runes on the arch read,\n\"Speak the key and you may pass.\"\nYou speak, and nothing happens." ),
                Dialog::OK );
        }
    }

    void ActionToTravellersTent( const Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        DEBUG_LOG( DBG_GAME, DBG_INFO, hero.GetName() )

        AudioManager::PlaySound( M82::EXPERNCE );

        fheroes2::showStandardTextMessage(
            MP2::StringObject( objectType ),
            _( "You enter the tent and see an old woman gazing into a magic gem. She looks up and says,\n\"In my travels, I have learned much in the way of arcane magic. A great oracle taught me his skill. I have the answer you seek.\"" ),
            Dialog::OK );

        const Maps::Tiles & tile = world.GetTiles( dst_index );
        Kingdom & kingdom = hero.GetKingdom();

        kingdom.SetVisitTravelersTent( getColorFromTile( tile ) );
    }
}

void Heroes::ScoutRadar() const
{
    Interface::AdventureMap & I = Interface::AdventureMap::Get();

#if defined( WITH_DEBUG )
    if ( GetColor() != Color::NONE ) {
        const Player * player = Players::Get( GetColor() );
        assert( player != nullptr );

        // If player gave control to AI we need to fully update the radar image as there is no need to make a code for rendering optimizations so we
        // don't call 'SetRenderArea()'.
        if ( !player->isAIAutoControlMode() ) {
#endif

            I.getRadar().SetRenderArea( GetScoutRoi() );

#if defined( WITH_DEBUG )
        }
    }
#endif

    I.setRedraw( Interface::REDRAW_RADAR );
}

void Heroes::Action( int tileIndex )
{
    // Hero may be lost while performing the action, reset the focus after completing the action (and update environment sounds and music if necessary)
    struct FocusUpdater
    {
        FocusUpdater() = default;

        FocusUpdater( const FocusUpdater & ) = delete;

        ~FocusUpdater()
        {
            Interface::AdventureMap & I = Interface::AdventureMap::Get();

            I.ResetFocus( GameFocus::HEROES, true );
            I.RedrawFocus();
        }

        FocusUpdater & operator=( const FocusUpdater & ) = delete;
    };

    std::unique_ptr<FocusUpdater> focusUpdater;

#if defined( WITH_DEBUG )
    const Player * player = Players::Get( GetKingdom().GetColor() );
    assert( player != nullptr );

    const bool isAIAutoControlMode = player->isAIAutoControlMode();
#else
    const bool isAIAutoControlMode = false;
#endif

    if ( !GetKingdom().isControlAI() || isAIAutoControlMode ) {
        focusUpdater = std::make_unique<FocusUpdater>();

        if ( isAIAutoControlMode ) {
            Interface::AdventureMap::Get().SetFocus( this, false );
        }
    }

    if ( GetKingdom().isControlAI() ) {
        // Restore the original music after the action is completed.
        const AudioManager::MusicRestorer musicRestorer;

        return AI::HeroesAction( *this, tileIndex );
    }

    const int32_t heroPosIndex = GetIndex();
    assert( heroPosIndex >= 0 );

    // Update environment sounds and music before performing the action
    if ( Game::UpdateSoundsOnFocusUpdate() ) {
        Game::EnvironmentSoundMixer();
        AudioManager::PlayMusicAsync( MUS::FromGround( world.GetTiles( heroPosIndex ).GetGround() ), Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );
    }

    const MP2::MapObjectType objectType = world.GetTiles( tileIndex ).GetObject( tileIndex != heroPosIndex );
    if ( MP2::isInGameActionObject( objectType, isShipMaster() ) ) {
        SetModes( ACTION );
    }

    // Most likely there will be some action or event, immediately center the map on the hero to avoid subsequent minor screen movements
    if ( Modes( ACTION ) || objectType == MP2::OBJ_EVENT ) {
        Interface::AdventureMap & I = Interface::AdventureMap::Get();

        I.getGameArea().SetCenter( GetCenter() );
        I.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR_CURSOR | Interface::REDRAW_HEROES );
    }

    switch ( objectType ) {
    case MP2::OBJ_MONSTER:
        ActionToMonster( *this, tileIndex );
        break;

    case MP2::OBJ_CASTLE:
        ActionToCastle( *this, tileIndex );
        break;
    case MP2::OBJ_HERO:
        ActionToHeroes( *this, tileIndex );
        break;

    case MP2::OBJ_BOAT:
        ActionToBoat( *this, tileIndex );
        break;
    case MP2::OBJ_COAST:
        ActionToCoast( *this, tileIndex );
        break;

    // resource object
    case MP2::OBJ_WINDMILL:
    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_MAGIC_GARDEN:
    case MP2::OBJ_LEAN_TO:
        ActionToObjectResource( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_WAGON:
        ActionToWagon( *this, tileIndex );
        break;
    case MP2::OBJ_SKELETON:
        ActionToSkeleton( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_BOTTLE:
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_RESOURCE:
        ActionToPickupResource( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST:
        ActionToTreasureChest( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_GENIE_LAMP:
        ActionToGenieLamp( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_FLOTSAM:
        ActionToFlotSam( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_SHIPWRECK_SURVIVOR:
        ActionToShipwreckSurvivor( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_ARTIFACT:
        ActionToArtifact( *this, tileIndex );
        break;

    // shrine circle
    case MP2::OBJ_SHRINE_FIRST_CIRCLE:
    case MP2::OBJ_SHRINE_SECOND_CIRCLE:
    case MP2::OBJ_SHRINE_THIRD_CIRCLE:
        ActionToShrine( *this, tileIndex );
        break;

    case MP2::OBJ_WITCHS_HUT:
        ActionToWitchsHut( *this, objectType, tileIndex );
        break;

    // info message
    case MP2::OBJ_SIGN:
        ActionToSign( *this, tileIndex );
        break;

    // luck modification
    case MP2::OBJ_FOUNTAIN:
    case MP2::OBJ_FAERIE_RING:
    case MP2::OBJ_IDOL:
        ActionToGoodLuckObject( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_PYRAMID:
        ActionToPyramid( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_MAGIC_WELL:
        ActionToMagicWell( *this, tileIndex );
        break;
    case MP2::OBJ_TRADING_POST:
        ActionToTradingPost( *this );
        break;

    // primary skill modification
    case MP2::OBJ_FORT:
    case MP2::OBJ_MERCENARY_CAMP:
    case MP2::OBJ_WITCH_DOCTORS_HUT:
    case MP2::OBJ_STANDING_STONES:
        ActionToPrimarySkillObject( *this, objectType, tileIndex );
        break;

    // morale modification
    case MP2::OBJ_OASIS:
    case MP2::OBJ_TEMPLE:
    case MP2::OBJ_WATERING_HOLE:
    case MP2::OBJ_BUOY:
        ActionToGoodMoraleObject( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_SHIPWRECK:
    case MP2::OBJ_GRAVEYARD:
    case MP2::OBJ_DERELICT_SHIP:
        ActionToPoorMoraleObject( *this, objectType, tileIndex );
        break;

    // experience modification
    case MP2::OBJ_GAZEBO:
        ActionToExperienceObject( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_DAEMON_CAVE:
        ActionToDaemonCave( *this, objectType, tileIndex );
        break;

    // teleports
    case MP2::OBJ_STONE_LITHS:
        ActionToTeleports( *this, tileIndex );
        break;
    case MP2::OBJ_WHIRLPOOL:
        ActionToWhirlpools( *this, tileIndex );
        break;

    case MP2::OBJ_OBSERVATION_TOWER:
        ActionToObservationTower( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_MAGELLANS_MAPS:
        ActionToMagellanMaps( *this, objectType, tileIndex );
        break;

    // capture color object
    case MP2::OBJ_ALCHEMIST_LAB:
    case MP2::OBJ_MINE:
    case MP2::OBJ_SAWMILL:
    case MP2::OBJ_LIGHTHOUSE:
        ActionToCaptureObject( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_ABANDONED_MINE:
        ActionToAbandonedMine( *this, objectType, tileIndex );
        break;

    // accept army
    case MP2::OBJ_WATCH_TOWER:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_TREE_HOUSE:
    case MP2::OBJ_ARCHER_HOUSE:
    case MP2::OBJ_GOBLIN_HUT:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_HALFLING_HOLE:
    case MP2::OBJ_PEASANT_HUT:
        ActionToDwellingJoinMonster( *this, objectType, tileIndex );
        break;

    // recruit army
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREE_CITY:
    case MP2::OBJ_WAGON_CAMP:
    case MP2::OBJ_DESERT_TENT:
        ActionToDwellingRecruitMonster( *this, objectType, tileIndex );
        break;

    // battle and recruit army
    case MP2::OBJ_DRAGON_CITY:
    case MP2::OBJ_CITY_OF_DEAD:
    case MP2::OBJ_TROLL_BRIDGE:
        ActionToDwellingBattleMonster( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_ARTESIAN_SPRING:
        ActionToArtesianSpring( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_XANADU:
        ActionToXanadu( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_HILL_FORT:
    case MP2::OBJ_FREEMANS_FOUNDRY:
        ActionToUpgradeArmyObject( *this, objectType, "" );
        break;

    case MP2::OBJ_EVENT:
        ActionToEvent( *this, tileIndex );
        break;

    case MP2::OBJ_OBELISK:
        ActionToObelisk( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_TREE_OF_KNOWLEDGE:
        ActionToTreeKnowledge( *this, objectType, tileIndex );
        break;

    case MP2::OBJ_ORACLE:
        ActionToOracle( *this, objectType );
        break;
    case MP2::OBJ_SPHINX:
        ActionToSphinx( *this, objectType, tileIndex );
        break;

    // loyalty version
    case MP2::OBJ_WATER_ALTAR:
    case MP2::OBJ_AIR_ALTAR:
    case MP2::OBJ_FIRE_ALTAR:
    case MP2::OBJ_EARTH_ALTAR:
    case MP2::OBJ_BARROW_MOUNDS:
        ActionToDwellingRecruitMonster( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_ALCHEMIST_TOWER:
        ActionToAlchemistTower( *this );
        break;
    case MP2::OBJ_STABLES:
        ActionToStables( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_ARENA:
        ActionToArena( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_MERMAID:
        ActionToGoodLuckObject( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_SIRENS:
        ActionToSirens( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_JAIL:
        ActionToJail( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_HUT_OF_MAGI:
        ActionToHutMagi( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_EYE_OF_MAGI:
        ActionToEyeMagi( *this, objectType );
        break;

    case MP2::OBJ_BARRIER:
        ActionToBarrier( *this, objectType, tileIndex );
        break;
    case MP2::OBJ_TRAVELLER_TENT:
        ActionToTravellersTent( *this, objectType, tileIndex );
        break;

    // other object
    default:
        break;
    }
}
