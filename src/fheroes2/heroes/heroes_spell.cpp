/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cassert>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "army.h"
#include "army_troop.h"
#include "audio_manager.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "direction.h"
#include "game_interface.h"
#include "heroes.h" // IWYU pragma: associated
#include "interface_base.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "kingdom.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "resource.h"
#include "route.h"
#include "settings.h"
#include "spell.h"
#include "spell_info.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "view_world.h"
#include "world.h"

namespace
{
    void HeroesTownGate( Heroes & hero, const Castle * castle )
    {
        assert( castle && castle->GetHero() == nullptr );

        Interface::AdventureMap & I = Interface::AdventureMap::Get();

        const fheroes2::Point fromPosition = hero.GetCenter();
        // Position of Hero on radar before casting the spell to clear it after casting.
        const fheroes2::Rect fromRoi( fromPosition.x, fromPosition.y, 1, 1 );

        // Before casting the spell, make sure that the game area is centered on the hero
        I.getGameArea().SetCenter( fromPosition );
        I.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR_CURSOR );

        const int32_t dst = castle->GetIndex();
        assert( Maps::isValidAbsIndex( dst ) );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.ShowPath( false );
        hero.FadeOut();

        hero.Scout( dst );
        hero.Move2Dest( dst );
        hero.GetPath().Reset();

        // Clear previous hero position on radar.
        I.getRadar().SetRenderArea( fromRoi );
        I.redraw( Interface::REDRAW_RADAR );

        I.getGameArea().SetCenter( hero.GetCenter() );

        // Update radar image in scout area around Hero after teleport.
        I.getRadar().SetRenderArea( hero.GetScoutRoi() );
        I.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.ShowPath( true );

        castle->MageGuildEducateHero( hero );
    }

    bool ActionSpellViewMines()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewMines, Interface::AdventureMap::Get() );
        return true;
    }

    bool ActionSpellViewResources()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewResources, Interface::AdventureMap::Get() );
        return true;
    }

    bool ActionSpellViewArtifacts()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewArtifacts, Interface::AdventureMap::Get() );
        return true;
    }

    bool ActionSpellViewTowns()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewTowns, Interface::AdventureMap::Get() );
        return true;
    }

    bool ActionSpellViewHeroes()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewHeroes, Interface::AdventureMap::Get() );
        return true;
    }

    bool ActionSpellViewAll()
    {
        ViewWorld::ViewWorldWindow( Settings::Get().CurrentColor(), ViewWorldMode::ViewAll, Interface::AdventureMap::Get() );
        return true;
    }

    bool ActionSpellIdentifyHero( const Heroes & hero )
    {
        assert( !hero.GetKingdom().Modes( Kingdom::IDENTIFYHERO ) );

        hero.GetKingdom().SetModes( Kingdom::IDENTIFYHERO );
        fheroes2::showStandardTextMessage( _( "Identify Hero" ), _( "Enemy heroes are now fully identifiable." ), Dialog::OK );

        return true;
    }

    bool ActionSpellSummonBoat( const Heroes & hero )
    {
        assert( !hero.isShipMaster() );

        const int32_t boatDestination = fheroes2::getPossibleBoatPosition( hero );
        assert( Maps::isValidAbsIndex( boatDestination ) );

        const int32_t boatSource = fheroes2::getSummonableBoat( hero );

        // Player should have a summonable boat before calling this function.
        assert( Maps::isValidAbsIndex( boatSource ) );

        const int heroColor = hero.GetColor();

        Interface::GameArea & gameArea = Interface::AdventureMap::Get().getGameArea();

        Maps::Tile & tileSource = world.getTile( boatSource );

        gameArea.runSingleObjectAnimation( std::make_shared<Interface::ObjectFadingOutInfo>( tileSource.getMainObjectPart()._uid, boatSource, MP2::OBJ_BOAT ) );

        Maps::Tile & tileDest = world.getTile( boatDestination );

        tileDest.setBoat( Direction::RIGHT, heroColor );
        tileSource.resetBoatOwnerColor();

        gameArea.runSingleObjectAnimation( std::make_shared<Interface::ObjectFadingInInfo>( tileDest.getMainObjectPart()._uid, boatDestination, MP2::OBJ_BOAT ) );

        return true;
    }

    bool ActionSpellDimensionDoor( Heroes & hero )
    {
        Interface::AdventureMap & I = Interface::AdventureMap::Get();

        const fheroes2::Point fromPosition = hero.GetCenter();
        // Position of Hero on radar before casting the spell to clear it after casting.
        const fheroes2::Rect fromRoi( fromPosition.x, fromPosition.y, 1, 1 );

        // Before casting the spell, make sure that the game area is centered on the hero
        I.getGameArea().SetCenter( hero.GetCenter() );
        I.redraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR_CURSOR );

        const int32_t src = hero.GetIndex();
        assert( Maps::isValidAbsIndex( src ) );

        const int32_t dst = I.GetDimensionDoorDestination( src, Spell::CalculateDimensionDoorDistance(), hero.isShipMaster() );
        if ( !Maps::isValidAbsIndex( dst ) ) {
            return false;
        }

        AudioManager::PlaySound( M82::KILLFADE );
        hero.ShowPath( false );
        hero.FadeOut();

        hero.Scout( dst );
        hero.Move2Dest( dst );
        hero.SpellCasted( Spell::DIMENSIONDOOR );
        hero.GetPath().Reset();

        // Clear previous hero position on radar.
        I.getRadar().SetRenderArea( fromRoi );
        I.redraw( Interface::REDRAW_RADAR );

        I.getGameArea().SetCenter( hero.GetCenter() );

        // Update radar image in scout area around Hero after teleport.
        I.getRadar().SetRenderArea( hero.GetScoutRoi() );
        I.setRedraw( Interface::REDRAW_GAMEAREA | Interface::REDRAW_RADAR );

        AudioManager::PlaySound( M82::KILLFADE );
        hero.FadeIn();
        hero.ShowPath( true );

        hero.ActionNewPosition( false );

        // SpellCasted() has already been called, we should not call it once again
        return false;
    }

    bool ActionSpellTownGate( Heroes & hero )
    {
        assert( !hero.isShipMaster() );

        const Castle * castle = fheroes2::getNearestCastleTownGate( hero );
        if ( !castle ) {
            // A hero must be able to have a destination castle. Something is wrong with the logic!
            assert( 0 );
            return false;
        }

        if ( castle->GetHero() ) {
            // The nearest town occupation must be checked before casting this spell. Something is wrong with the logic!
            assert( 0 );
            return false;
        }

        HeroesTownGate( hero, castle );

        return true;
    }

    bool ActionSpellTownPortal( Heroes & hero )
    {
        assert( !hero.isShipMaster() );

        const int32_t townPositionIndex = Dialog::selectKingdomCastle( hero.GetKingdom(), true, _( "Town Portal" ), _( "Select town to port to." ) );

        if ( townPositionIndex == -1 ) {
            return false;
        }

        HeroesTownGate( hero, world.getCastleEntrance( Maps::GetPoint( townPositionIndex ) ) );

        return true;
    }

    bool ActionSpellVisions( Heroes & hero )
    {
        const MapsIndexes monsters = Maps::getVisibleMonstersAroundHero( hero );

        assert( !monsters.empty() );

        for ( const int32_t monsterIndex : monsters ) {
            const Maps::Tile & tile = world.getTile( monsterIndex );

            const Troop troop = getTroopFromTile( tile );
            const NeutralMonsterJoiningCondition join = Army::GetJoinSolution( hero, tile, troop );

            std::string hdr;
            std::string msg;

            hdr = std::string( "%{count} " ) + troop.GetPluralName( join.monsterCount );
            StringReplace( hdr, "%{count}", troop.GetCount() );

            switch ( join.reason ) {
            case NeutralMonsterJoiningCondition::Reason::Free:
            case NeutralMonsterJoiningCondition::Reason::Alliance:
                msg = _( "The creatures are willing to join us!" );
                break;

            case NeutralMonsterJoiningCondition::Reason::ForMoney:
                if ( join.monsterCount == troop.GetCount() )
                    msg = _( "All the creatures will join us..." );
                else {
                    msg = _n( "The creature will join us...", "%{count} of the creatures will join us...", join.monsterCount );
                    StringReplace( msg, "%{count}", join.monsterCount );
                }

                msg += '\n';
                msg.append( _( "\n for a fee of %{gold} gold." ) );
                StringReplace( msg, "%{gold}", troop.GetTotalCost().gold );
                break;

            case NeutralMonsterJoiningCondition::Reason::RunAway:
            case NeutralMonsterJoiningCondition::Reason::Bane:
                msg = _( "These weak creatures will surely flee before us." );
                break;
            default:
                msg = _( "I fear these creatures are in the mood for a fight." );
                break;
            }

            fheroes2::showStandardTextMessage( hdr, msg, Dialog::OK );
        }

        hero.SetModes( Heroes::VISIONS );

        return true;
    }

    bool ActionSpellSetGuardian( const Heroes & hero, const Spell & spell )
    {
        Maps::Tile & tile = world.getTile( hero.GetIndex() );
        assert( MP2::OBJ_MINE == tile.getMainObjectType( false ) );

        const uint32_t count = fheroes2::getGuardianMonsterCount( spell, hero.GetPower(), &hero );

        if ( count == 0 ) {
            return false;
        }

        Maps::setMineSpellOnTile( tile, spell.GetID() );

        if ( spell == Spell::HAUNT ) {
            world.CaptureObject( tile.GetIndex(), Color::NONE );

            // Update the color of haunted mine on radar.
            Interface::AdventureMap & I = Interface::AdventureMap::Get();
            const fheroes2::Point heroPosition = hero.GetCenter();
            I.getRadar().SetRenderArea( { heroPosition.x - 1, heroPosition.y - 1, 3, 2 } );

            I.setRedraw( Interface::REDRAW_RADAR );
        }

        world.GetCapturedObject( tile.GetIndex() ).GetTroop().Set( Monster( spell ), count );

        return true;
    }
}

void Heroes::ActionSpellCast( const Spell & spell )
{
    assert( spell.isValid() && !spell.isCombat() && CanCastSpell( spell ) );

    bool apply = false;

    switch ( spell.GetID() ) {
    case Spell::VIEWMINES:
        apply = ActionSpellViewMines();
        break;
    case Spell::VIEWRESOURCES:
        apply = ActionSpellViewResources();
        break;
    case Spell::VIEWARTIFACTS:
        apply = ActionSpellViewArtifacts();
        break;
    case Spell::VIEWTOWNS:
        apply = ActionSpellViewTowns();
        break;
    case Spell::VIEWHEROES:
        apply = ActionSpellViewHeroes();
        break;
    case Spell::VIEWALL:
        apply = ActionSpellViewAll();
        break;
    case Spell::IDENTIFYHERO:
        apply = ActionSpellIdentifyHero( *this );
        break;
    case Spell::SUMMONBOAT:
        apply = ActionSpellSummonBoat( *this );
        break;
    case Spell::DIMENSIONDOOR:
        apply = ActionSpellDimensionDoor( *this );
        break;
    case Spell::TOWNGATE:
        apply = ActionSpellTownGate( *this );
        break;
    case Spell::TOWNPORTAL:
        apply = ActionSpellTownPortal( *this );
        break;
    case Spell::VISIONS:
        apply = ActionSpellVisions( *this );
        break;
    case Spell::HAUNT:
    case Spell::SETEGUARDIAN:
    case Spell::SETAGUARDIAN:
    case Spell::SETFGUARDIAN:
    case Spell::SETWGUARDIAN:
        apply = ActionSpellSetGuardian( *this, spell );
        break;
    default:
        break;
    }

    if ( !apply ) {
        return;
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << " cast spell: " << spell.GetName() )

    SpellCasted( spell );
}
