/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <cstdlib>

#include "agg_image.h"
#include "army.h"
#include "castle.h"
#include "castle_ui.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "kingdom.h"
#include "profit.h"
#include "settings.h"
#include "text.h"
#include "tools.h"
#include "world.h"

std::string GetMinesIncomeString( int type )
{
    const payment_t income = ProfitConditions::FromMine( type );
    const s32 value = income.Get( type );
    std::string res;

    if ( value ) {
        res.append( " " );
        res.append( "(" );
        res.append( value > 0 ? "+" : "-" );
        res.append( std::to_string( value ) );
        res.append( ")" );
    }

    return res;
}

std::string ShowGuardiansInfo( const Maps::Tiles & tile, bool isOwned, bool extendedScoutingOption, uint32_t basicScoutingLevel )
{
    std::string str;
    const Troop & troop = tile.QuantityTroop();

    if ( MP2::OBJ_MINES == tile.GetObject() ) {
        str = Maps::GetMinesName( tile.QuantityResourceCount().first );
        str.append( GetMinesIncomeString( tile.QuantityResourceCount().first ) );
    }
    else if ( tile.GetObject() == MP2::OBJN_ABANDONEDMINE || tile.GetObject() == MP2::OBJ_ABANDONEDMINE ) {
        const uint8_t spriteIndex = tile.GetObjectSpriteIndex();
        if ( spriteIndex == 5 ) { // TODO: remove this hardocded value for real abandoned mine.
            str = MP2::StringObject( tile.GetObject() );
        }
        else {
            str = Maps::GetMinesName( tile.QuantityResourceCount().first );
        }
    }
    else {
        str = MP2::StringObject( tile.GetObject() );
    }

    if ( troop.isValid() && ( isOwned || ( extendedScoutingOption && basicScoutingLevel > Skill::Level::NONE ) ) ) {
        str.append( "\n \n" );
        str.append( _( "guarded by %{count} %{monster}" ) );

        const int scoutingLevel = isOwned ? static_cast<int>( Skill::Level::EXPERT ) : basicScoutingLevel;
        StringReplace( str, "%{count}", StringLower( Game::CountScoute( troop.GetCount(), scoutingLevel ) ) );
        if ( troop.GetCount() == 1 && scoutingLevel == Skill::Level::EXPERT ) {
            StringReplace( str, "%{monster}", StringLower( troop.GetName() ) );
        }
        else {
            StringReplace( str, "%{monster}", StringLower( troop.GetMultiName() ) );
        }
    }

    return str;
}

std::string ShowMonsterInfo( const Maps::Tiles & tile, bool isVisibleFromCrystalBall, bool extendedScoutingOption, uint32_t basicScoutingLevel )
{
    const Troop & troop = tile.QuantityTroop();

    if ( isVisibleFromCrystalBall || ( extendedScoutingOption && basicScoutingLevel > Skill::Level::NONE ) ) {
        std::string str = "%{count} %{monster}";

        const int scoutingLevel = isVisibleFromCrystalBall ? static_cast<int>( Skill::Level::EXPERT ) : basicScoutingLevel;
        StringReplace( str, "%{count}", Game::CountScoute( troop.GetCount(), scoutingLevel ) );
        if ( troop.GetCount() == 1 && scoutingLevel == Skill::Level::EXPERT ) {
            StringReplace( str, "%{monster}", StringLower( troop.GetName() ) );
        }
        else {
            StringReplace( str, "%{monster}", StringLower( troop.GetMultiName() ) );
        }

        return str;
    }
    else {
        return Army::TroopSizeString( troop );
    }
}

std::string ShowArtifactInfo( const Maps::Tiles & tile, bool extendedScoutingOption, uint32_t scoutingLevel )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    if ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) {
        str.append( "\n(" );
        str.append( tile.QuantityArtifact().GetName() );
        str.append( ")" );
    }

    return str;
}

std::string ShowResourceInfo( const Maps::Tiles & tile, bool extendedScoutingOption, uint32_t scoutingLevel )
{
    std::string str;

    if ( MP2::OBJ_RESOURCE == tile.GetObject() ) {
        str = Resource::String( tile.GetQuantity1() );
        if ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) {
            const ResourceCount & rc = tile.QuantityResourceCount();
            str.append( ": " );
            str.append( Game::CountScoute( rc.second, scoutingLevel ) );
        }
    }
    else { // Campfire
        str = MP2::StringObject( tile.GetObject() );
        if ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) {
            const Funds & funds = tile.QuantityFunds();

            str.append( "\n(" );
            str.append( Resource::String( Resource::GOLD ) );

            str.append( ": " );
            str.append( Game::CountScoute( funds.gold, scoutingLevel ) );
            str.append( "\n" );

            const ResourceCount & rc = tile.QuantityResourceCount();
            str.append( Resource::String( rc.first ) );

            str.append( ": " );
            str.append( Game::CountScoute( rc.second, scoutingLevel ) );
            str.append( ")" );
        }
    }

    return str;
}

std::string ShowDwellingInfo( const Maps::Tiles & tile, bool owned, bool extendedScoutingOption, uint32_t scoutingLevel )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    if ( owned || ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) ) {
        str.append( "\n" );
        const Troop & troop = tile.QuantityTroop();
        if ( troop.isValid() ) {
            str.append( _( "(available: %{count})" ) );
            StringReplace( str, "%{count}", Game::CountScoute( troop.GetCount(), owned ? static_cast<int>( Skill::Level::EXPERT ) : scoutingLevel ) );
        }
        else
            str.append( "(empty)" );
    }

    return str;
}

std::string ShowShrineInfo( const Maps::Tiles & tile, const Heroes * hero, bool showVisitedOption, bool isVisited, bool extendedScoutingOption, uint32_t scoutingLevel )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    bool showSpellDetails = false;

    if ( showVisitedOption && isVisited ) {
        showSpellDetails = true;
    }
    else if ( extendedScoutingOption ) {
        switch ( tile.GetObject() ) {
        case MP2::OBJ_SHRINE1:
            showSpellDetails = scoutingLevel >= Skill::Level::BASIC;
            break;
        case MP2::OBJ_SHRINE2:
            showSpellDetails = scoutingLevel >= Skill::Level::ADVANCED;
            break;
        case MP2::OBJ_SHRINE3:
            showSpellDetails = scoutingLevel == Skill::Level::EXPERT;
            break;
        default:
            break;
        }
    }

    if ( showSpellDetails ) {
        const Spell & spell = tile.QuantitySpell();
        str.append( "\n(" );
        str.append( spell.GetName() );
        str.append( ")" );
        if ( hero && hero->HaveSpell( spell ) ) {
            str.append( "\n(" );
            str.append( _( "already learned" ) );
            str.append( ")" );
        }
    }

    return str;
}

std::string ShowWitchHutInfo( const Maps::Tiles & tile, const Heroes * hero, bool showVisitedOption, bool isVisited, bool extendedScoutingOption, uint32_t scoutingLevel )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    const bool show = ( showVisitedOption && isVisited ) || ( extendedScoutingOption && scoutingLevel == Skill::Level::EXPERT );

    if ( show ) {
        const Skill::Secondary & skill = tile.QuantitySkill();
        str.append( "\n(" );
        str.append( Skill::Secondary::String( skill.Skill() ) );
        str.append( ")" );

        if ( hero ) {
            if ( hero->HasSecondarySkill( skill.Skill() ) ) {
                str.append( "\n(" );
                str.append( _( "already knows this skill" ) );
                str.append( ")" );
            }
            else if ( hero->HasMaxSecondarySkill() ) {
                str.append( "\n(" );
                str.append( _( "already has max skills" ) );
                str.append( ")" );
            }
        }
    }

    return str;
}

std::string ShowLocalVisitTileInfo( const Maps::Tiles & tile, const Heroes * hero )
{
    std::string str = MP2::StringObject( tile.GetObject() );
    if ( hero ) {
        str.append( "\n \n" );
        str.append( hero->isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );
    }

    return str;
}

std::string ShowLocalVisitObjectInfo( const Maps::Tiles & tile, const Heroes * hero )
{
    std::string str = MP2::StringObject( tile.GetObject() );
    if ( hero ) {
        str.append( "\n \n" );
        str.append( hero->isObjectTypeVisited( tile.GetObject() ) ? _( "(already visited)" ) : _( "(not visited)" ) );
    }

    return str;
}

std::string ShowGlobalVisitInfo( const Maps::Tiles & tile, const Kingdom & kingdom, bool showVisitedOption )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    if ( showVisitedOption ) {
        str.append( "\n \n" );
        str.append( kingdom.isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );
    }

    return str;
}

std::string ShowBarrierTentInfo( const Maps::Tiles & tile, const Kingdom & kingdom )
{
    std::string str = BarrierColor::String( tile.QuantityColor() );
    str.append( " " );
    str.append( MP2::StringObject( tile.GetObject() ) );

    if ( MP2::OBJ_TRAVELLERTENT == tile.GetObject() && kingdom.IsVisitTravelersTent( tile.QuantityColor() ) ) {
        str.append( "\n \n" );
        str.append( _( "(already visited)" ) );
    }

    return str;
}

std::string ShowGroundInfo( const Maps::Tiles & tile, const bool showTerrainPenaltyOption, const Heroes * hero )
{
    const int objectType = tile.GetObject( false );

    std::string str;
    if ( objectType == MP2::OBJ_COAST ) {
        str = MP2::StringObject( objectType );
    }
    else if ( tile.isRoad() ) {
        str = _( "Road" );
    }
    else {
        str = Maps::Ground::String( tile.GetGround() );
    }

    str.append( "\n \n" );

    if ( tile.GoodForUltimateArtifact() ) {
        str.append( _( "(digging ok)" ) );
    }
    else {
        str.append( _( "(no digging)" ) );
    }

    if ( showTerrainPenaltyOption && hero ) {
        const uint32_t cost = tile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, hero->GetLevelSkill( Skill::Secondary::PATHFINDING ) );
        if ( cost > 0 ) {
            str.append( "\n" );
            str.append( _( "penalty: %{cost}" ) );
            StringReplace( str, "%{cost}", cost );
        }
    }

    return str;
}

fheroes2::Rect MakeRectQuickInfo( const LocalEvent & le, const fheroes2::Sprite & imageBox, const fheroes2::Point & position = fheroes2::Point() )
{
    if ( position.x > 0 && position.y > 0 ) {
        return fheroes2::Rect( position.x - imageBox.width(), position.y, imageBox.width(), imageBox.height() );
    }

    // place box next to mouse cursor
    const fheroes2::Point & mp = le.GetMouseCursor();

    const int32_t mx = ( ( mp.x - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;
    const int32_t my = ( ( mp.y - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;

    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const fheroes2::Rect & ar = gamearea.GetROI();

    int32_t xpos = mx + TILEWIDTH - ( imageBox.width() / 2 );
    int32_t ypos = my + TILEWIDTH - ( imageBox.height() / 2 );

    // clamp box to edges of adventure screen game area
    xpos = clamp( xpos, BORDERWIDTH, ( ar.width - imageBox.width() ) + BORDERWIDTH );
    ypos = clamp( ypos, BORDERWIDTH, ( ar.height - imageBox.height() ) + BORDERWIDTH );

    return fheroes2::Rect( xpos, ypos, imageBox.width(), imageBox.height() );
}

uint32_t GetHeroScoutingLevelForTile( const Heroes * hero, uint32_t dst )
{
    if ( hero == nullptr ) {
        return Skill::Level::NONE;
    }

    const uint32_t scoutingLevel = hero->GetSecondaryValues( Skill::Secondary::SCOUTING );
    const int tileObject = world.GetTiles( dst ).GetObject();

    const bool monsterInfo = tileObject == MP2::OBJ_MONSTER;

    // TODO check that this logic is what is really intended, it's only used for extended scouting anyway
    if ( monsterInfo ) {
        if ( Maps::GetApproximateDistance( hero->GetIndex(), dst ) <= hero->GetVisionsDistance() ) {
            return scoutingLevel;
        }
        else {
            return Skill::Level::NONE;
        }
    }
    else if ( Settings::Get().ExtWorldScouteExtended() ) {
        uint32_t dist = static_cast<uint32_t>( hero->GetScoute() );
        if ( hero->Modes( Heroes::VISIONS ) && dist < hero->GetVisionsDistance() )
            dist = hero->GetVisionsDistance();

        if ( Maps::GetApproximateDistance( hero->GetIndex(), dst ) <= dist )
            return scoutingLevel;
        return Skill::Level::NONE;
    }

    return Skill::Level::NONE;
}

void Dialog::QuickInfo( const Maps::Tiles & tile )
{
    const int objectType = tile.GetObject( false );

    // check
    switch ( objectType ) {
    case MP2::OBJN_MINES:
    case MP2::OBJN_ABANDONEDMINE:
    case MP2::OBJN_SAWMILL:
    case MP2::OBJN_ALCHEMYLAB: {
        const Maps::Tiles & left = world.GetTiles( tile.GetIndex() - 1 );
        const Maps::Tiles & right = world.GetTiles( tile.GetIndex() + 1 );
        const Maps::Tiles * center = nullptr;

        if ( MP2::isGroundObject( left.GetObject( false ) ) )
            center = &left;
        else if ( MP2::isGroundObject( right.GetObject( false ) ) )
            center = &right;

        if ( center ) {
            QuickInfo( *center );
            return;
        }
    } break;

    default:
        break;
    }

    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    const Settings & settings = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();

    // preload
    const int qwikinfo = ICN::QWIKINFO;

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( qwikinfo, 0 );

    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect pos = MakeRectQuickInfo( le, box );

    fheroes2::ImageRestorer restorer( display, pos.x, pos.y, pos.width, pos.height );
    fheroes2::Blit( box, display, pos.x, pos.y );

    std::string name_object;

    const Heroes * from_hero = Interface::GetFocusHeroes();
    const Kingdom & kingdom = world.GetKingdom( settings.CurrentColor() );

    const bool isVisibleFromCrystalBall = kingdom.IsTileVisibleFromCrystalBall( tile.GetIndex() );

    // This value is only relevant for the "Extended Scouting" option
    const uint32_t scoutingLevelForTile = isVisibleFromCrystalBall ? static_cast<int>( Skill::Level::EXPERT ) : GetHeroScoutingLevelForTile( from_hero, tile.GetIndex() );

    const bool showVisitedOption = settings.ExtWorldShowVisitedContent();
    const bool showTerrainPenaltyOption = settings.ExtWorldShowTerrainPenalty();
    const bool extendedScoutingOption = settings.ExtWorldScouteExtended();

    if ( tile.isFog( settings.CurrentColor() ) )
        name_object = _( "Uncharted Territory" );
    else
        // check guardians mine
        if ( MP2::OBJ_ABANDONEDMINE == objectType || tile.CaptureObjectIsProtection() ) {
        name_object = ShowGuardiansInfo( tile, settings.CurrentColor() == tile.QuantityColor(), extendedScoutingOption, scoutingLevelForTile );
    }
    else
        switch ( objectType ) {
        case MP2::OBJ_MONSTER:
            name_object = ShowMonsterInfo( tile, isVisibleFromCrystalBall, extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_EVENT:
        case MP2::OBJ_ZERO:
        case MP2::OBJ_COAST:
            name_object = ShowGroundInfo( tile, showTerrainPenaltyOption, from_hero );
            break;

        case MP2::OBJ_DERELICTSHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DAEMONCAVE:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEANTO:
            name_object = ShowGlobalVisitInfo( tile, kingdom, showVisitedOption );
            break;

        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_MAGICGARDEN:
            name_object = Settings::Get().ExtWorldExtObjectsCaptured() ? MP2::StringObject( objectType ) : ShowGlobalVisitInfo( tile, kingdom, showVisitedOption );
            break;

        case MP2::OBJ_CAMPFIRE:
            name_object = ShowResourceInfo( tile, extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_RESOURCE:
            name_object = ShowResourceInfo( tile, extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_ARTIFACT:
            name_object = ShowArtifactInfo( tile, extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_MINES:
            name_object = Maps::GetMinesName( tile.QuantityResourceCount().first );
            if ( settings.CurrentColor() == tile.QuantityColor() )
                name_object.append( GetMinesIncomeString( tile.QuantityResourceCount().first ) );
            break;

        case MP2::OBJ_ALCHEMYLAB:
        case MP2::OBJ_SAWMILL:
            name_object = MP2::StringObject( objectType );
            if ( settings.CurrentColor() == tile.QuantityColor() )
                name_object.append( GetMinesIncomeString( tile.QuantityResourceCount().first ) );
            break;

        // join army
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_PEASANTHUT:
        case MP2::OBJ_THATCHEDHUT:
        // recruit army
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
        // battle and recruit army
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
        case MP2::OBJ_TROLLBRIDGE:
            name_object = ShowDwellingInfo( tile, kingdom.isVisited( tile ), extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_GAZEBO:
        case MP2::OBJ_FORT:
        case MP2::OBJ_XANADU:
        case MP2::OBJ_MERCENARYCAMP:
        case MP2::OBJ_DOCTORHUT:
        case MP2::OBJ_STANDINGSTONES:
        case MP2::OBJ_TREEKNOWLEDGE:
            name_object = ShowLocalVisitTileInfo( tile, from_hero );
            break;

        case MP2::OBJ_ARTESIANSPRING:
            name_object = ShowGlobalVisitInfo( tile, kingdom, true );
            break;

        case MP2::OBJ_MAGICWELL:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_FAERIERING:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_OASIS:
        case MP2::OBJ_TEMPLE:
        case MP2::OBJ_BUOY:
        case MP2::OBJ_MERMAID:
        case MP2::OBJ_WATERINGHOLE:
        case MP2::OBJ_ARENA:
        case MP2::OBJ_STABLES:
        case MP2::OBJ_SIRENS:
            name_object = ShowLocalVisitObjectInfo( tile, from_hero );
            break;

        case MP2::OBJ_SHRINE1:
        case MP2::OBJ_SHRINE2:
        case MP2::OBJ_SHRINE3:
            name_object = ShowShrineInfo( tile, from_hero, showVisitedOption, kingdom.isVisited( tile ), extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_WITCHSHUT:
            name_object = ShowWitchHutInfo( tile, from_hero, showVisitedOption, kingdom.isVisited( tile ), extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_OBELISK:
            name_object = ShowGlobalVisitInfo( tile, kingdom, true );
            break;

        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLERTENT:
            name_object = ShowBarrierTentInfo( tile, kingdom );
            break;

        default:
            name_object = MP2::StringObject( objectType );
            break;
        }

    TextBox text( name_object, Font::SMALL, 118 );
    text.Blit( pos.x + BORDERWIDTH + ( pos.width - BORDERWIDTH - text.w() ) / 2, pos.y + ( pos.height - BORDERWIDTH - text.h() ) / 2 );

    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    restorer.restore();
    display.render();
}

void Dialog::QuickInfo( const Castle & castle, const fheroes2::Point & position /*= fheroes2::Point()*/ )
{
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::QWIKTOWN, 0 );

    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect cur_rt = MakeRectQuickInfo( le, box, position );

    fheroes2::ImageRestorer back( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( cur_rt.x + 22, cur_rt.y + 9, 192, 154 );
    fheroes2::Point dst_pt;
    Text text;

    // castle name
    text.Set( castle.GetName(), Font::SMALL );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
    dst_pt.y = cur_rt.y;
    text.Blit( dst_pt.x, dst_pt.y );

    // castle icon
    const Settings & conf = Settings::Get();
    const fheroes2::Sprite & castleIcon = fheroes2::AGG::GetICN( conf.ExtGameEvilInterface() ? ICN::LOCATORE : ICN::LOCATORS, 23 );

    dst_pt.x = cur_rt.x + ( cur_rt.width - castleIcon.width() ) / 2;
    dst_pt.y += 11;
    fheroes2::Blit( castleIcon, display, dst_pt.x, dst_pt.y );
    fheroes2::drawCastleIcon( castle, display, fheroes2::Point( dst_pt.x + 4, dst_pt.y + 4 ) );

    // color flags
    uint32_t index = 0;
    switch ( castle.GetColor() ) {
    case Color::BLUE:
        index = 0;
        break;
    case Color::GREEN:
        index = 2;
        break;
    case Color::RED:
        index = 4;
        break;
    case Color::YELLOW:
        index = 6;
        break;
    case Color::ORANGE:
        index = 8;
        break;
    case Color::PURPLE:
        index = 10;
        break;
    case Color::NONE:
        index = 12;
        break;
    default:
        break;
    }

    const fheroes2::Point flagOffset( 5, 4 );

    const fheroes2::Sprite & l_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index );
    fheroes2::Blit( l_flag, display, dst_pt.x - flagOffset.x - l_flag.width(), dst_pt.y + flagOffset.y );

    const fheroes2::Sprite & r_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index + 1 );
    fheroes2::Blit( r_flag, display, dst_pt.x + flagOffset.x + castleIcon.width(), dst_pt.y + flagOffset.y );

    const int currentColor = conf.CurrentColor();
    const Kingdom & kingdom = world.GetKingdom( currentColor );

    const bool isFriend = castle.isFriends( currentColor );
    const bool isVisibleFromCrystalBall = kingdom.IsTileVisibleFromCrystalBall( castle.GetIndex() );

    uint32_t scoutSkillLevel = Skill::Level::NONE;

    if ( isFriend || isVisibleFromCrystalBall ) {
        scoutSkillLevel = Skill::Level::EXPERT;
    }
    else {
        scoutSkillLevel = std::min( kingdom.GetCountThievesGuild(), static_cast<uint32_t>( Skill::Level::EXPERT ) );
    }

    const Heroes * guardian = castle.GetHeroes().Guard();
    const bool isGuardianVisible = guardian && scoutSkillLevel >= Skill::Level::ADVANCED;

    // show guardian
    if ( isGuardianVisible ) {
        // hero name
        text.Set( guardian->GetName(), Font::SMALL );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += castleIcon.height() + 5;
        text.Blit( dst_pt.x, dst_pt.y );

        // hero avatar
        const fheroes2::Sprite & port = guardian->GetPortrait( PORT_SMALL );
        if ( !port.empty() ) {
            dst_pt.x = cur_rt.x + ( cur_rt.width - port.width() ) / 2;
            dst_pt.y += 15;
            fheroes2::Blit( port, display, dst_pt.x, dst_pt.y );
        }
    }
    else {
        text.Set( _( "Defenders:" ) );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += castleIcon.height() + 5;
        text.Blit( dst_pt.x, dst_pt.y );
    }

    const uint32_t count = castle.GetArmy().GetCount();

    // draw defenders
    if ( count == 0 ) {
        text.Set( _( "None" ) );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += 45;
        text.Blit( dst_pt.x, dst_pt.y );
    }
    else if ( scoutSkillLevel > Skill::Level::NONE ) {
        const bool isScouteView = isFriend || isVisibleFromCrystalBall;

        dst_pt.x = cur_rt.x - 5;
        dst_pt.y += 20;

        Army::DrawMonsterLines( castle.GetArmy(), dst_pt.x, dst_pt.y, 192, scoutSkillLevel, isGuardianVisible, isScouteView );
    }
    else {
        text.Set( _( "Unknown" ) );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += 45;
        text.Blit( dst_pt.x, dst_pt.y );
    }

    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    back.restore();
    display.render();
}

void Dialog::QuickInfo( const Heroes & hero, const fheroes2::Point & position /*= fheroes2::Point()*/ )
{
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    const int qwikhero = ICN::QWIKHERO;

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( qwikhero, 0 );

    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect cur_rt = MakeRectQuickInfo( le, box, position );

    fheroes2::ImageRestorer restorer( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( restorer.x() + 28, restorer.y() + 10, 146, 144 );
    fheroes2::Point dst_pt;
    Text text;
    std::string message;

    const Kingdom & kingdom = world.GetKingdom( conf.CurrentColor() );
    const bool isFriend = hero.isFriends( conf.CurrentColor() );
    const bool isUnderIdentifyHeroSpell = kingdom.Modes( Kingdom::IDENTIFYHERO );
    const bool showFullInfo = isFriend || isUnderIdentifyHeroSpell || kingdom.IsTileVisibleFromCrystalBall( hero.GetIndex() );

    // heroes name
    if ( showFullInfo ) {
        message = _( "%{name} (Level %{level})" );
        StringReplace( message, "%{name}", hero.GetName() );
        StringReplace( message, "%{level}", hero.GetLevel() );
    }
    else
        message = hero.GetName();
    text.Set( message, Font::SMALL );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
    dst_pt.y = cur_rt.y;
    text.Blit( dst_pt.x, dst_pt.y );

    // mini port heroes
    const fheroes2::Sprite & port = hero.GetPortrait( PORT_SMALL );
    if ( !port.empty() ) {
        dst_pt.x = cur_rt.x + ( cur_rt.width - port.width() ) / 2;
        dst_pt.y = cur_rt.y + 13;
        fheroes2::Blit( port, display, dst_pt.x, dst_pt.y );
    }

    // luck
    if ( showFullInfo ) {
        const s32 luck = hero.GetLuckWithModificators( nullptr );
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINILKMR, ( 0 > luck ? 0 : ( 0 < luck ? 1 : 2 ) ) );
        u32 count = ( 0 == luck ? 1 : std::abs( luck ) );
        dst_pt.x = cur_rt.x + 120;
        dst_pt.y = cur_rt.y + ( count == 1 ? 20 : 13 );

        while ( count-- ) {
            fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
            dst_pt.y += sprite.height() - 1;
        }
    }

    // morale
    if ( showFullInfo ) {
        const s32 morale = hero.GetMoraleWithModificators( nullptr );
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINILKMR, ( 0 > morale ? 3 : ( 0 < morale ? 4 : 5 ) ) );
        u32 count = ( 0 == morale ? 1 : std::abs( morale ) );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y = cur_rt.y + ( count == 1 ? 20 : 13 );

        while ( count-- ) {
            fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
            dst_pt.y += sprite.height() - 1;
        }
    }

    // color flags
    u32 index = 0;

    switch ( hero.GetColor() ) {
    case Color::BLUE:
        index = 0;
        break;
    case Color::GREEN:
        index = 2;
        break;
    case Color::RED:
        index = 4;
        break;
    case Color::YELLOW:
        index = 6;
        break;
    case Color::ORANGE:
        index = 8;
        break;
    case Color::PURPLE:
        index = 10;
        break;
    case Color::NONE:
        index = 12;
        break;
    default:
        break;
    }

    dst_pt.y = cur_rt.y + 13;

    const fheroes2::Sprite & l_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index );
    dst_pt.x = cur_rt.x + ( cur_rt.width - 40 ) / 2 - l_flag.width();
    fheroes2::Blit( l_flag, display, dst_pt.x, dst_pt.y );

    const fheroes2::Sprite & r_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index + 1 );
    dst_pt.x = cur_rt.x + ( cur_rt.width + 40 ) / 2;
    fheroes2::Blit( r_flag, display, dst_pt.x, dst_pt.y );

    if ( showFullInfo ) {
        // attack
        text.Set( std::string( _( "Attack" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += port.height();
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( std::to_string( hero.GetAttack() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // defense
        text.Set( std::string( _( "Defense" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( std::to_string( hero.GetDefense() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // power
        text.Set( std::string( _( "Spell Power" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( std::to_string( hero.GetPower() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // knowledge
        text.Set( std::string( _( "Knowledge" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( std::to_string( hero.GetKnowledge() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // spell point
        text.Set( std::string( _( "Spell Points" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( std::to_string( hero.GetSpellPoints() ) + "/" + std::to_string( hero.GetMaxSpellPoints() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // move point
        text.Set( std::string( _( "Move Points" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( std::to_string( hero.GetMovePoints() ) + "/" + std::to_string( hero.GetMaxMovePoints() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        Army::DrawMons32Line( hero.GetArmy(), cur_rt.x - 7, cur_rt.y + 116, 160 );
    }
    else {
        // show limited
        Army::DrawMonsterLines( hero.GetArmy(), cur_rt.x - 6, cur_rt.y + 60, 160, Skill::Level::NONE, false, true );
    }

    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    restorer.restore();
    display.render();
}
