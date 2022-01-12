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
#include "translations.h"
#include "ui_text.h"
#include "world.h"

#include <cassert>

namespace
{
    class RadarUpdater
    {
    public:
        RadarUpdater( const fheroes2::Rect & mainArea, const fheroes2::Point & updatedPosition )
            : _mainArea( mainArea )
            , _updatedPosition( updatedPosition )
            , _prevPosition( Interface::Basic::Get().GetGameArea().getCurrentCenterInPixels() )
            , _restorer( fheroes2::Display::instance(), 0, 0, 0, 0 )
        {
            if ( _updatedPosition != _prevPosition ) {
                Interface::Radar & radar = Interface::Basic::Get().GetRadar();

                const fheroes2::Rect commonArea = mainArea ^ radar.GetRect();
                _restorer.update( commonArea.x, commonArea.y, commonArea.width, commonArea.height );

                Interface::Basic::Get().GetGameArea().SetCenter( updatedPosition );
                radar.Redraw();

                _restorer.restore();
            }
        }

        void restore()
        {
            if ( _updatedPosition != _prevPosition ) {
                Interface::Basic::Get().GetGameArea().SetCenterInPixels( _prevPosition );
                Interface::Basic::Get().GetRadar().Redraw();

                _restorer.restore();
            }
        }

    private:
        const fheroes2::Rect _mainArea;
        const fheroes2::Point _updatedPosition;
        const fheroes2::Point _prevPosition;
        fheroes2::ImageRestorer _restorer;
    };

    std::string GetMinesIncomeString( const int resourceType )
    {
        const payment_t income = ProfitConditions::FromMine( resourceType );
        const int32_t value = income.Get( resourceType );

        std::string res;
        if ( value == 0 ) {
            return res;
        }

        res += ' ';
        res += '(';
        res += ( value > 0 ? '+' : '-' );
        res.append( std::to_string( value ) );
        res += ')';

        return res;
    }

    std::string ShowGuardiansInfo( const Maps::Tiles & tile, bool isOwned, bool extendedScoutingOption, uint32_t basicScoutingLevel )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );
        const bool isAbandonnedMine = ( objectType == MP2::OBJN_ABANDONEDMINE || objectType == MP2::OBJ_ABANDONEDMINE );

        std::string str;
        if ( MP2::OBJ_MINES == objectType ) {
            str = Maps::GetMinesName( tile.QuantityResourceCount().first );
            str.append( GetMinesIncomeString( tile.QuantityResourceCount().first ) );
        }
        else if ( isAbandonnedMine ) {
            const uint8_t spriteIndex = tile.GetObjectSpriteIndex();
            if ( spriteIndex == 5 ) { // TODO: remove this hardocded value for real abandoned mine.
                str = MP2::StringObject( objectType );
            }
            else {
                str = Maps::GetMinesName( tile.QuantityResourceCount().first );
            }
        }
        else {
            str = MP2::StringObject( objectType );
        }

        const Troop & troop = tile.QuantityTroop();
        if ( troop.isValid() && ( isOwned || isAbandonnedMine || ( extendedScoutingOption && basicScoutingLevel > Skill::Level::NONE ) ) ) {
            str.append( "\n \n" );
            const int scoutingLevel = isOwned ? static_cast<int>( Skill::Level::EXPERT ) : basicScoutingLevel;
            if ( scoutingLevel == Skill::Level::NONE ) {
                str.append( _( "guarded by " ) ).append( StringLower( Army::TroopSizeString( troop ) ) );
            }
            else {
                str.append( _( "guarded by %{count} %{monster}" ) );
                StringReplace( str, "%{count}", StringLower( Game::CountScoute( troop.GetCount(), scoutingLevel ) ) );
            }
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
        std::string str = MP2::StringObject( tile.GetObject( false ) );

        if ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) {
            str.append( "\n(" );
            str.append( tile.QuantityArtifact().GetName() );
            str += ')';
        }

        return str;
    }

    std::string ShowResourceInfo( const Maps::Tiles & tile, bool extendedScoutingOption, uint32_t scoutingLevel )
    {
        std::string str;

        const MP2::MapObjectType objectType = tile.GetObject( false );

        if ( MP2::OBJ_RESOURCE == objectType ) {
            str = Resource::String( tile.GetQuantity1() );
            if ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) {
                const ResourceCount & rc = tile.QuantityResourceCount();
                str.append( ": " );
                str.append( Game::CountScoute( rc.second, scoutingLevel ) );
            }
        }
        else { // Campfire
            str = MP2::StringObject( objectType );
            if ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) {
                const Funds & funds = tile.QuantityFunds();

                str.append( "\n(" );
                str.append( Resource::String( Resource::GOLD ) );

                str.append( ": " );
                str.append( Game::CountScoute( funds.gold, scoutingLevel ) );
                str += '\n';

                const ResourceCount & rc = tile.QuantityResourceCount();
                str.append( Resource::String( rc.first ) );

                str.append( ": " );
                str.append( Game::CountScoute( rc.second, scoutingLevel ) );
                str += ')';
            }
        }

        return str;
    }

    std::string ShowDwellingInfo( const Maps::Tiles & tile, bool owned, bool extendedScoutingOption, uint32_t scoutingLevel )
    {
        std::string str = MP2::StringObject( tile.GetObject( false ) );

        if ( owned || ( extendedScoutingOption && scoutingLevel > Skill::Level::NONE ) ) {
            str += '\n';
            const Troop & troop = tile.QuantityTroop();
            if ( troop.isValid() ) {
                str.append( _( "(available: %{count})" ) );
                StringReplace( str, "%{count}", Game::CountScoute( troop.GetCount(), owned ? static_cast<int>( Skill::Level::EXPERT ) : scoutingLevel ) );
            }
            else {
                str.append( _( "(empty)" ) );
            }
        }

        return str;
    }

    std::string ShowShrineInfo( const Maps::Tiles & tile, const Heroes * hero, bool isVisited, bool extendedScoutingOption, uint32_t scoutingLevel )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );

        std::string str = MP2::StringObject( objectType );

        bool showSpellDetails = false;

        if ( isVisited ) {
            showSpellDetails = true;
        }
        else if ( extendedScoutingOption ) {
            switch ( objectType ) {
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
            str += ')';
            if ( hero && hero->HaveSpell( spell ) ) {
                str.append( "\n(" );
                str.append( _( "already learned" ) );
                str += ')';
            }
        }

        return str;
    }

    std::string ShowWitchHutInfo( const Maps::Tiles & tile, const Heroes * hero, bool isVisited, bool extendedScoutingOption, uint32_t scoutingLevel )
    {
        std::string str = MP2::StringObject( tile.GetObject( false ) );

        const bool show = isVisited || ( extendedScoutingOption && scoutingLevel == Skill::Level::EXPERT );

        if ( show ) {
            const Skill::Secondary & skill = tile.QuantitySkill();
            str.append( "\n(" );
            str.append( Skill::Secondary::String( skill.Skill() ) );
            str += ')';

            if ( hero ) {
                if ( hero->HasSecondarySkill( skill.Skill() ) ) {
                    str.append( "\n(" );
                    str.append( _( "already knows this skill" ) );
                    str += ')';
                }
                else if ( hero->HasMaxSecondarySkill() ) {
                    str.append( "\n(" );
                    str.append( _( "already has max skills" ) );
                    str += ')';
                }
            }
        }

        return str;
    }

    std::string ShowLocalVisitTileInfo( const Maps::Tiles & tile, const Heroes * hero )
    {
        std::string str = MP2::StringObject( tile.GetObject( false ) );
        if ( hero ) {
            str.append( "\n \n" );
            str.append( hero->isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );
        }

        return str;
    }

    std::string ShowLocalVisitObjectInfo( const Maps::Tiles & tile, const Heroes * hero )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );

        std::string str = MP2::StringObject( objectType );
        if ( hero ) {
            str.append( "\n \n" );
            str.append( hero->isObjectTypeVisited( objectType ) ? _( "(already visited)" ) : _( "(not visited)" ) );
        }

        return str;
    }

    std::string ShowGlobalVisitInfo( const Maps::Tiles & tile, const Kingdom & kingdom )
    {
        std::string str = MP2::StringObject( tile.GetObject( false ) );

        str.append( "\n \n" );
        str.append( kingdom.isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );

        return str;
    }

    std::string showUniqueObjectVisitInfo( const MP2::MapObjectType objectType, const Kingdom & kingdom )
    {
        std::string str = MP2::StringObject( objectType );

        str.append( "\n \n" );
        str.append( kingdom.isVisited( objectType ) ? _( "(already visited)" ) : _( "(not visited)" ) );

        return str;
    }

    std::string ShowBarrierInfo( const Maps::Tiles & tile )
    {
        std::string str = _( "%{color} Barrier" );
        StringReplace( str, "%{color}", fheroes2::getBarrierColorName( tile.QuantityColor() ) );

        return str;
    }

    std::string ShowTentInfo( const Maps::Tiles & tile, const Kingdom & kingdom )
    {
        std::string str = _( "%{color} Tent" );
        StringReplace( str, "%{color}", fheroes2::getTentColorName( tile.QuantityColor() ) );

        if ( kingdom.IsVisitTravelersTent( tile.QuantityColor() ) ) {
            str.append( "\n \n" );
            str.append( _( "(already visited)" ) );
        }

        return str;
    }

    std::string ShowGroundInfo( const Maps::Tiles & tile, const bool showTerrainPenaltyOption, const Heroes * hero )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );

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
                str += '\n';
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
        const MP2::MapObjectType objectType = world.GetTiles( dst ).GetObject( false );

        const bool monsterInfo = objectType == MP2::OBJ_MONSTER;

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
}

void Dialog::QuickInfo( const Maps::Tiles & tile, const bool ignoreHeroOnTile )
{
    const MP2::MapObjectType objectType = tile.GetObject( !ignoreHeroOnTile );
    const MP2::MapObjectType correctedObjectType = MP2::getBaseActionObjectType( objectType );

    if ( objectType != correctedObjectType && MP2::isActionObject( correctedObjectType ) && !ignoreHeroOnTile ) {
        const int32_t mainTileIndex = Maps::Tiles::getIndexOfMainTile( tile );
        if ( mainTileIndex != -1 ) {
            QuickInfo( world.GetTiles( mainTileIndex ), true );
            return;
        }
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
            name_object = ShowGlobalVisitInfo( tile, kingdom );
            break;
        case MP2::OBJ_MAGELLANMAPS:
            name_object = showUniqueObjectVisitInfo( objectType, kingdom );
            break;
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_MAGICGARDEN:
            name_object = Settings::Get().ExtWorldExtObjectsCaptured() ? MP2::StringObject( objectType ) : ShowGlobalVisitInfo( tile, kingdom );
            break;

        case MP2::OBJ_CAMPFIRE:
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
        case MP2::OBJ_BARROWMOUNDS:
        case MP2::OBJ_AIRALTAR:
        case MP2::OBJ_FIREALTAR:
        case MP2::OBJ_EARTHALTAR:
        case MP2::OBJ_WATERALTAR:
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
            name_object = ShowGlobalVisitInfo( tile, kingdom );
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
            name_object = ShowShrineInfo( tile, from_hero, kingdom.isVisited( tile ), extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_WITCHSHUT:
            name_object = ShowWitchHutInfo( tile, from_hero, kingdom.isVisited( tile ), extendedScoutingOption, scoutingLevelForTile );
            break;

        case MP2::OBJ_OBELISK:
            name_object = ShowGlobalVisitInfo( tile, kingdom );
            break;

        case MP2::OBJ_BARRIER:
            name_object = ShowBarrierInfo( tile );
            break;

        case MP2::OBJ_TRAVELLERTENT:
            name_object = ShowTentInfo( tile, kingdom );
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

void Dialog::QuickInfo( const Castle & castle, const fheroes2::Rect & activeArea, const fheroes2::Point & position /*= fheroes2::Point()*/ )
{
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    // Update radar.
    RadarUpdater radarUpdater( activeArea, castle.GetCenter() );

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::QWIKTOWN, 0 );

    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect cur_rt = MakeRectQuickInfo( le, box, position );

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::ImageRestorer back( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( cur_rt.x + 22, cur_rt.y + 9, 192, 154 );
    fheroes2::Point dst_pt;
    fheroes2::Text text;
    const fheroes2::FontType smallWhiteFont{ fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE };

    // castle name
    text.set( castle.GetName(), smallWhiteFont );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
    dst_pt.y = cur_rt.y + 3;
    text.draw( dst_pt.x, dst_pt.y, display );

    // castle icon
    const Settings & conf = Settings::Get();
    const fheroes2::Sprite & castleIcon = fheroes2::AGG::GetICN( conf.ExtGameEvilInterface() ? ICN::LOCATORE : ICN::LOCATORS, 23 );

    dst_pt.x = cur_rt.x + ( cur_rt.width - castleIcon.width() ) / 2;
    dst_pt.y += 10;
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
        text.set( guardian->GetName(), smallWhiteFont );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
        dst_pt.y += castleIcon.height() + 5;
        text.draw( dst_pt.x, dst_pt.y, display );

        // hero avatar
        const fheroes2::Sprite & port = guardian->GetPortrait( PORT_SMALL );
        if ( !port.empty() ) {
            dst_pt.x = cur_rt.x + ( cur_rt.width - port.width() ) / 2;
            dst_pt.y += 15;
            fheroes2::Blit( port, display, dst_pt.x, dst_pt.y );
        }
    }
    else {
        text.set( _( "Defenders:" ), smallWhiteFont );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
        dst_pt.y += castleIcon.height() + 2;
        text.draw( dst_pt.x, dst_pt.y, display );
    }

    const uint32_t count = castle.GetArmy().GetCount();

    // draw defenders
    if ( count == 0 ) {
        text.set( _( "None" ), smallWhiteFont );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
        dst_pt.y += 47;
        text.draw( dst_pt.x, dst_pt.y, display );
    }
    else if ( scoutSkillLevel > Skill::Level::NONE ) {
        const bool isScouteView = isFriend || isVisibleFromCrystalBall;

        dst_pt.x = cur_rt.x - 1;
        dst_pt.y += 21;

        Army::DrawMonsterLines( castle.GetArmy(), dst_pt.x, dst_pt.y, 192, scoutSkillLevel, isGuardianVisible, isScouteView );
    }
    else {
        text.set( _( "Unknown" ), smallWhiteFont );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
        dst_pt.y += 47;
        text.draw( dst_pt.x, dst_pt.y, display );
    }

    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    back.restore();

    // Restore radar view.
    radarUpdater.restore();

    display.render();
}

void Dialog::QuickInfo( const Heroes & hero, const fheroes2::Rect & activeArea, const fheroes2::Point & position /*= fheroes2::Point()*/ )
{
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    // Update radar.
    RadarUpdater radarUpdater( activeArea, hero.GetCenter() );

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

        Army::DrawMons32Line( hero.GetArmy(), cur_rt.x - 7, cur_rt.y + 117, 160 );
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

    // Restore radar view.
    radarUpdater.restore();

    display.render();
}
