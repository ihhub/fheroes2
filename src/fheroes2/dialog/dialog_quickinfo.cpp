/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "artifact_ultimate.h"
#include "captain.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "pairs.h"
#include "payment.h"
#include "profit.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_castle.h"
#include "ui_text.h"
#include "world.h"

namespace
{
    void outputInTextSupportMode( const Maps::Tiles & tile, const std::string & info )
    {
        START_TEXT_SUPPORT_MODE

        const int tileIndex = tile.GetIndex();
        const int mapWidth = world.w();
        const int x = tileIndex % mapWidth;
        const int y = tileIndex / mapWidth;

        COUT( "[" << x + 1 << ", " << y + 1 << "]" )
        COUT( info )
    }

    class RadarUpdater
    {
    public:
        RadarUpdater( const bool performUpdate, const fheroes2::Point & updatedPosition, const fheroes2::Rect & areaToRestore )
            : _performUpdate( performUpdate )
            , _updatedPosition( updatedPosition )
            , _prevPosition( Interface::Basic::Get().GetGameArea().getCurrentCenterInPixels() )
            , _restorer( fheroes2::Display::instance(), areaToRestore.x, areaToRestore.y, areaToRestore.width, areaToRestore.height )
        {
            if ( !_performUpdate || _updatedPosition == _prevPosition ) {
                return;
            }

            Interface::Basic & iface = Interface::Basic::Get();

            iface.GetGameArea().SetCenter( updatedPosition );
            iface.Redraw( Interface::REDRAW_RADAR );

            _restorer.restore();
        }

        void restore()
        {
            if ( !_performUpdate || _updatedPosition == _prevPosition ) {
                return;
            }

            Interface::Basic & iface = Interface::Basic::Get();

            iface.GetGameArea().SetCenterInPixels( _prevPosition );
            iface.Redraw( Interface::REDRAW_RADAR );

            _restorer.restore();
        }

    private:
        const bool _performUpdate;
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

    std::string ShowGuardiansInfo( const Maps::Tiles & tile, bool isOwned )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );
        const bool isAbandonedMine = ( objectType == MP2::OBJN_ABANDONEDMINE || objectType == MP2::OBJ_ABANDONEDMINE );

        std::string str;

        if ( MP2::OBJ_MINES == objectType ) {
            str = Maps::GetMinesName( tile.QuantityResourceCount().first );
            str.append( GetMinesIncomeString( tile.QuantityResourceCount().first ) );
        }
        else if ( isAbandonedMine ) {
            const uint8_t spriteIndex = tile.GetObjectSpriteIndex();
            if ( spriteIndex == 5 ) { // TODO: remove this hardcoded value for real abandoned mine.
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
        if ( troop.isValid() && ( isOwned || isAbandonedMine ) ) {
            str.append( "\n \n" );

            if ( isOwned ) {
                str.append( _( "guarded by %{count} %{monster}" ) );
                StringReplaceWithLowercase( str, "%{count}", Game::formatMonsterCount( troop.GetCount(), true ) );

                if ( troop.GetCount() == 1 ) {
                    StringReplaceWithLowercase( str, "%{monster}", troop.GetName() );
                }
                else {
                    StringReplaceWithLowercase( str, "%{monster}", troop.GetMultiName() );
                }
            }
            else {
                str.append( _( "guarded by " ) ).append( Translation::StringLower( Army::TroopSizeString( troop ) ) );
            }
        }

        return str;
    }

    std::string ShowMonsterInfo( const Maps::Tiles & tile, const bool isVisibleFromCrystalBall )
    {
        const Troop & troop = tile.QuantityTroop();

        if ( isVisibleFromCrystalBall ) {
            std::string str = "%{count} %{monster}";
            StringReplace( str, "%{count}", Game::formatMonsterCount( troop.GetCount(), true ) );

            if ( troop.GetCount() == 1 ) {
                StringReplaceWithLowercase( str, "%{monster}", troop.GetName() );
            }
            else {
                StringReplaceWithLowercase( str, "%{monster}", troop.GetMultiName() );
            }

            return str;
        }
        else {
            return Army::TroopSizeString( troop );
        }
    }

    std::string ShowArtifactInfo( const Maps::Tiles & tile )
    {
        return MP2::StringObject( tile.GetObject( false ) );
    }

    std::string ShowResourceInfo( const Maps::Tiles & tile )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );

        if ( MP2::OBJ_RESOURCE == objectType ) {
            return Resource::String( tile.GetQuantity1() );
        }
        else { // Campfire
            return MP2::StringObject( objectType );
        }
    }

    std::string ShowDwellingInfo( const Maps::Tiles & tile, bool owned )
    {
        std::string str = MP2::StringObject( tile.GetObject( false ) );

        if ( owned ) {
            str += "\n \n";

            const Troop & troop = tile.QuantityTroop();

            if ( troop.isValid() ) {
                str.append( _( "(available: %{count})" ) );
                StringReplace( str, "%{count}", Game::formatMonsterCount( troop.GetCount(), true ) );
            }
            else {
                str.append( _( "(empty)" ) );
            }
        }

        return str;
    }

    std::string ShowShrineInfo( const Maps::Tiles & tile, const Heroes * hero, bool isVisited )
    {
        const MP2::MapObjectType objectType = tile.GetObject( false );

        std::string str = MP2::StringObject( objectType );

        if ( isVisited ) {
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

    std::string ShowWitchHutInfo( const Maps::Tiles & tile, const Heroes * hero, bool isVisited )
    {
        std::string str = MP2::StringObject( tile.GetObject( false ) );

        if ( isVisited ) {
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

    std::string ShowGroundInfo( const Maps::Tiles & tile, const Heroes * hero )
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

        // Original Editor allows to put an Ultimate Artifact on an invalid tile. So checking tile index solves this issue.
        if ( tile.GoodForUltimateArtifact() || world.GetUltimateArtifact().getPosition() == tile.GetIndex() ) {
            str.append( _( "(digging ok)" ) );
        }
        else {
            str.append( _( "(no digging)" ) );
        }

        if ( hero ) {
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
            return { position.x - imageBox.width(), position.y, imageBox.width(), imageBox.height() };
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
        assert( ar.width >= imageBox.width() && ar.height >= imageBox.height() );
        xpos = std::clamp( xpos, BORDERWIDTH, ( ar.width - imageBox.width() ) + BORDERWIDTH );
        ypos = std::clamp( ypos, BORDERWIDTH, ( ar.height - imageBox.height() ) + BORDERWIDTH );

        return { xpos, ypos, imageBox.width(), imageBox.height() };
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

    if ( tile.isFog( settings.CurrentColor() ) )
        name_object = _( "Uncharted Territory" );
    else if ( MP2::OBJ_ABANDONEDMINE == objectType || tile.isCaptureObjectProtected() ) {
        name_object = ShowGuardiansInfo( tile, settings.CurrentColor() == tile.QuantityColor() );
    }
    else
        switch ( objectType ) {
        case MP2::OBJ_MONSTER:
            name_object = ShowMonsterInfo( tile, isVisibleFromCrystalBall );
            break;

        case MP2::OBJ_EVENT:
        case MP2::OBJ_ZERO:
        case MP2::OBJ_COAST:
            name_object = ShowGroundInfo( tile, from_hero );
            break;

        case MP2::OBJ_DERELICTSHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DAEMONCAVE:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEANTO:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_MAGICGARDEN:
            name_object = ShowGlobalVisitInfo( tile, kingdom );
            break;
        case MP2::OBJ_MAGELLANMAPS:
            name_object = showUniqueObjectVisitInfo( objectType, kingdom );
            break;

        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_RESOURCE:
            name_object = ShowResourceInfo( tile );
            break;

        case MP2::OBJ_ARTIFACT:
            name_object = ShowArtifactInfo( tile );
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
            name_object = ShowDwellingInfo( tile, kingdom.isVisited( tile ) );
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
            name_object = ShowShrineInfo( tile, from_hero, kingdom.isVisited( tile ) );
            break;

        case MP2::OBJ_WITCHSHUT:
            name_object = ShowWitchHutInfo( tile, from_hero, kingdom.isVisited( tile ) );
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

    const int32_t objectTextBorderedWidth = pos.width - 2 * BORDERWIDTH;
    const fheroes2::Text text( name_object, fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 22, pos.y - 6 + ( ( pos.height - text.height( objectTextBorderedWidth ) ) / 2 ), objectTextBorderedWidth, display );

    outputInTextSupportMode( tile, name_object );

    display.render( restorer.rect() );

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    restorer.restore();
    display.render( restorer.rect() );
}

void Dialog::QuickInfo( const Castle & castle, const fheroes2::Point & position /* = {} */, const bool showOnRadar /* = false */,
                        const fheroes2::Rect & areaToRestore /* = {} */ )
{
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    // Update radar if needed
    RadarUpdater radarUpdater( showOnRadar, castle.GetCenter(), areaToRestore );

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::QWIKTOWN, 0 );

    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect cur_rt = MakeRectQuickInfo( le, box, position );

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::ImageRestorer back( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( cur_rt.x + 22, cur_rt.y + 9, 192, 154 );
    fheroes2::Point dst_pt;

    // castle name
    fheroes2::Text text( castle.GetName(), fheroes2::FontType::smallWhite() );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
    dst_pt.y = cur_rt.y + 3;
    text.draw( dst_pt.x, dst_pt.y, display );

    // castle icon
    const Settings & conf = Settings::Get();
    const fheroes2::Sprite & castleIcon = fheroes2::AGG::GetICN( conf.isEvilInterfaceEnabled() ? ICN::LOCATORE : ICN::LOCATORS, 23 );

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

    const bool isDetailedView = castle.isFriends( currentColor ) || kingdom.IsTileVisibleFromCrystalBall( castle.GetIndex() );
    const uint32_t thievesGuildsCount = kingdom.GetCountThievesGuild();

    text.set( _( "Defenders:" ), fheroes2::FontType::smallWhite() );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
    dst_pt.y += castleIcon.height() + 2;
    text.draw( dst_pt.x, dst_pt.y, display );

    const uint32_t count = castle.GetArmy().GetOccupiedSlotCount();

    // draw defenders
    if ( count == 0 ) {
        text.set( _( "None" ), fheroes2::FontType::smallWhite() );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
        dst_pt.y += 47;
        text.draw( dst_pt.x, dst_pt.y, display );
    }
    else if ( isDetailedView || thievesGuildsCount > 0 ) {
        dst_pt.x = cur_rt.x - 1;
        dst_pt.y += 21;

        Army::drawMultipleMonsterLines( castle.GetArmy(), dst_pt.x, dst_pt.y, 192, false, isDetailedView, true, thievesGuildsCount );
    }
    else {
        text.set( _( "Unknown" ), fheroes2::FontType::smallWhite() );
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

    // Restore radar view
    radarUpdater.restore();

    display.render();
}

void Dialog::QuickInfo( const HeroBase & hero, const fheroes2::Point & position /* = {} */, const bool showOnRadar /* = false */,
                        const fheroes2::Rect & areaToRestore /* = {} */ )
{
    const CursorRestorer cursorRestorer( false, Cursor::POINTER );

    // Update radar if needed
    RadarUpdater radarUpdater( showOnRadar, hero.GetCenter(), areaToRestore );

    const int qwikhero = ICN::QWIKHERO;

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( qwikhero, 0 );

    LocalEvent & le = LocalEvent::Get();
    fheroes2::Rect cur_rt = MakeRectQuickInfo( le, box, position );

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::ImageRestorer restorer( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( restorer.x() + 28, restorer.y() + 10, 146, 144 );
    fheroes2::Point dst_pt;

    const Settings & conf = Settings::Get();
    const Kingdom & kingdom = world.GetKingdom( conf.CurrentColor() );
    const bool isFriend = ColorBase( hero.GetColor() ).isFriends( conf.CurrentColor() );
    const bool isUnderIdentifyHeroSpell = kingdom.Modes( Kingdom::IDENTIFYHERO );
    const bool isNeutralHero = ( hero.GetColor() == Color::NONE );
    const bool showFullInfo = isNeutralHero || isFriend || isUnderIdentifyHeroSpell || kingdom.IsTileVisibleFromCrystalBall( hero.GetIndex() );

    const Heroes * activeHero = dynamic_cast<const Heroes *>( &hero );
    const Captain * activeCaptain = dynamic_cast<const Captain *>( &hero );
    assert( activeHero != nullptr || activeCaptain != nullptr );

    const bool isActiveHero = ( activeHero != nullptr );

    std::string message;
    // hero's name
    if ( showFullInfo && isActiveHero ) {
        message = _( "%{name} (Level %{level})" );
        StringReplace( message, "%{name}", hero.GetName() );
        StringReplace( message, "%{level}", activeHero->GetLevel() );
    }
    else {
        message = hero.GetName();
    }

    const fheroes2::FontType smallWhite = fheroes2::FontType::smallWhite();
    fheroes2::Text text( message, smallWhite );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
    dst_pt.y = cur_rt.y + 2;
    text.draw( dst_pt.x, dst_pt.y, display );

    // mini port heroes
    const fheroes2::Sprite & port = isActiveHero ? activeHero->GetPortrait( PORT_SMALL ) : activeCaptain->GetPortrait( PORT_SMALL );
    if ( !port.empty() ) {
        dst_pt.x = cur_rt.x + ( cur_rt.width - port.width() ) / 2;
        dst_pt.y = cur_rt.y + 13;
        fheroes2::Blit( port, display, dst_pt.x, dst_pt.y );
    }

    // luck
    if ( showFullInfo ) {
        const int32_t luck = hero.GetLuck();
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINILKMR, ( 0 > luck ? 0 : ( 0 < luck ? 1 : 2 ) ) );
        uint32_t count = ( 0 == luck ? 1 : std::abs( luck ) );
        dst_pt.x = cur_rt.x + 120;
        dst_pt.y = cur_rt.y + ( count == 1 ? 20 : 13 );

        while ( count-- ) {
            fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
            dst_pt.y += sprite.height() - 1;
        }
    }

    // morale
    if ( showFullInfo ) {
        const int32_t morale = hero.GetMorale();
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINILKMR, ( 0 > morale ? 3 : ( 0 < morale ? 4 : 5 ) ) );
        uint32_t count = ( 0 == morale ? 1 : std::abs( morale ) );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y = cur_rt.y + ( count == 1 ? 20 : 13 );

        while ( count-- ) {
            fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
            dst_pt.y += sprite.height() - 1;
        }
    }

    dst_pt.y = cur_rt.y + 13;

    // color flags, except for neutral heroes
    if ( !isNeutralHero ) {
        uint32_t index = 0;

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
        default:
            break;
        }

        const fheroes2::Sprite & l_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index );
        dst_pt.x = cur_rt.x + ( cur_rt.width - 40 ) / 2 - l_flag.width();
        fheroes2::Blit( l_flag, display, dst_pt.x, dst_pt.y );

        const fheroes2::Sprite & r_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index + 1 );
        dst_pt.x = cur_rt.x + ( cur_rt.width + 40 ) / 2;
        fheroes2::Blit( r_flag, display, dst_pt.x, dst_pt.y );
    }

    const uint16_t statNumberColumn = 89;
    const uint16_t statRow = 12;

    if ( showFullInfo ) {
        // attack
        text.set( _( "Attack:" ), smallWhite );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += port.height() + 2;
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( hero.GetAttack() ), smallWhite );
        dst_pt.x += statNumberColumn;
        text.draw( dst_pt.x, dst_pt.y, display );

        // defense
        text.set( _( "Defense:" ), smallWhite );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += statRow;
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( hero.GetDefense() ), smallWhite );
        dst_pt.x += statNumberColumn;
        text.draw( dst_pt.x, dst_pt.y, display );

        // power
        text.set( _( "Spell Power:" ), smallWhite );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += statRow;
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( hero.GetPower() ), smallWhite );
        dst_pt.x += statNumberColumn;
        text.draw( dst_pt.x, dst_pt.y, display );

        // knowledge
        text.set( _( "Knowledge:" ), smallWhite );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += statRow;
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( hero.GetKnowledge() ), smallWhite );
        dst_pt.x += statNumberColumn;
        text.draw( dst_pt.x, dst_pt.y, display );

        // spell point
        text.set( _( "Spell Points:" ), smallWhite );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += statRow;
        text.draw( dst_pt.x, dst_pt.y, display );

        text.set( std::to_string( hero.GetSpellPoints() ) + "/" + std::to_string( hero.GetMaxSpellPoints() ), smallWhite );
        dst_pt.x += statNumberColumn;
        text.draw( dst_pt.x, dst_pt.y, display );

        // move point
        if ( isActiveHero ) {
            text.set( _( "Move Points:" ), smallWhite );
            dst_pt.x = cur_rt.x + 10;
            dst_pt.y += statRow;
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( activeHero->GetMovePoints() ) + "/" + std::to_string( activeHero->GetMaxMovePoints() ), smallWhite );
            dst_pt.x += statNumberColumn;
            text.draw( dst_pt.x, dst_pt.y, display );
        }

        Army::drawSingleDetailedMonsterLine( hero.GetArmy(), cur_rt.x - 7, cur_rt.y + 117, 160 );
    }
    else {
        // show limited
        Army::drawMultipleMonsterLines( hero.GetArmy(), cur_rt.x - 6, cur_rt.y + 60, 160, false, false );
    }

    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    restorer.restore();

    // Restore radar view
    radarUpdater.restore();

    display.render();
}
