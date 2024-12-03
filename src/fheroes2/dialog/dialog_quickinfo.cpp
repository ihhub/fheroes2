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

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "artifact_ultimate.h"
#include "captain.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h" // IWYU pragma: associated
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "mp2.h"
#include "profit.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_castle.h"
#include "ui_map_interface.h"
#include "ui_text.h"
#include "world.h"

namespace
{
    void outputInTextSupportMode( const Maps::Tile & tile, const std::string & info )
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
            , _prevPosition( Interface::AdventureMap::Get().getGameArea().getCurrentCenterInPixels() )
            , _restorer( fheroes2::Display::instance(), areaToRestore.x, areaToRestore.y, areaToRestore.width, areaToRestore.height )
        {
            if ( !_performUpdate || _updatedPosition == _prevPosition ) {
                return;
            }

            Interface::AdventureMap & iface = Interface::AdventureMap::Get();

            iface.getGameArea().SetCenter( updatedPosition );
            iface.redraw( Interface::REDRAW_RADAR_CURSOR );

            _restorer.restore();
        }

        void restore()
        {
            if ( !_performUpdate || _updatedPosition == _prevPosition ) {
                return;
            }

            Interface::AdventureMap & iface = Interface::AdventureMap::Get();

            iface.getGameArea().SetCenterInPixels( _prevPosition );
            iface.redraw( Interface::REDRAW_RADAR_CURSOR );

            _restorer.restore();
        }

    private:
        const bool _performUpdate;
        const fheroes2::Point _updatedPosition;
        const fheroes2::Point _prevPosition;
        fheroes2::ImageRestorer _restorer;
    };

    std::string getMineIncomeString( const int32_t resourceType )
    {
        const Funds income = ProfitConditions::FromMine( resourceType );
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

    std::string showMineInfo( const Maps::Tile & tile, const bool isOwned )
    {
        const int32_t resourceType = getDailyIncomeObjectResources( tile ).getFirstValidResource().first;
        std::string objectInfo = Maps::GetMineName( resourceType );

        if ( isOwned ) {
            // TODO: we should use the value from funds.
            objectInfo.append( getMineIncomeString( resourceType ) );
        }

        return objectInfo;
    }

    std::string showGuardiansInfo( const Maps::Tile & tile, const bool isOwned )
    {
        const MP2::MapObjectType objectType = tile.getMainObjectType( false );

        std::string str;

        if ( objectType == MP2::OBJ_MINE ) {
            str = showMineInfo( tile, isOwned );
        }
        else {
            str = MP2::StringObject( objectType );
        }

        const Troop & troop = getTroopFromTile( tile );

        if ( troop.isValid() ) {
            str.append( "\n\n" );

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

    std::string showMonsterInfo( const Maps::Tile & tile, const bool isVisibleFromCrystalBall )
    {
        const Troop & troop = getTroopFromTile( tile );

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

        return Army::TroopSizeString( troop );
    }

    std::string showDwellingInfo( const Maps::Tile & tile, const bool isOwned )
    {
        std::string str = MP2::StringObject( tile.getMainObjectType( false ) );

        if ( isOwned ) {
            str += "\n\n";

            const Troop & troop = getTroopFromTile( tile );

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

    std::string showShrineInfo( const Maps::Tile & tile, const bool isVisited )
    {
        const MP2::MapObjectType objectType = tile.getMainObjectType( false );

        std::string str = MP2::StringObject( objectType );

        if ( isVisited ) {
            const Spell & spell = getSpellFromTile( tile );

            str.append( "\n\n(" );
            str.append( spell.GetName() );
            str += ')';

            const Heroes * hero = Interface::GetFocusHeroes();

            if ( hero && hero->HaveSpell( spell ) ) {
                str.append( "\n(" );
                str.append( _( "already learned" ) );
                str += ')';
            }
        }

        return str;
    }

    std::string showTreeOfKnowledgeInfo( const Maps::Tile & tile, const bool isVisited )
    {
        const MP2::MapObjectType objectType = tile.getMainObjectType( false );
        std::string str = MP2::StringObject( objectType );
        const Heroes * hero = Interface::GetFocusHeroes();

        if ( isVisited ) {
            const Funds & payment = getTreeOfKnowledgeRequirement( tile );
            str.append( "\n\n(" );
            if ( payment.GetValidItemsCount() == 0 ) {
                str.append( _( "treeOfKnowledge|free" ) );
            }
            else {
                const auto rc = payment.getFirstValidResource();
                str.append( std::to_string( rc.second ) );
                str += ' ';
                str.append( Translation::StringLower( Resource::String( rc.first ) ) );
            }
            str += ')';

            if ( hero ) {
                str.append( "\n(" );
                str.append( hero->isVisited( tile ) ? _( "already claimed" ) : _( "not claimed" ) );
                str += ')';
            }
        }
        else {
            if ( hero ) {
                str.append( "\n\n(" );
                // In case another player visited this tree with this hero.
                str.append( hero->isVisited( tile ) ? _( "already claimed" ) : _( "not claimed" ) );
                str += ')';
            }
        }

        return str;
    }

    std::string showWitchHutInfo( const Maps::Tile & tile, const bool isVisited )
    {
        std::string str = MP2::StringObject( tile.getMainObjectType( false ) );

        if ( isVisited ) {
            const Skill::Secondary & skill = getSecondarySkillFromWitchsHut( tile );

            str.append( "\n\n(" );
            str.append( Skill::Secondary::String( skill.Skill() ) );
            str += ')';

            const Heroes * hero = Interface::GetFocusHeroes();

            if ( hero ) {
                const char * heroStr = nullptr;

                if ( hero->HasSecondarySkill( skill.Skill() ) ) {
                    heroStr = ( _( "already knows this skill" ) );
                }
                else if ( hero->HasMaxSecondarySkill() ) {
                    heroStr = ( _( "already has max skills" ) );
                }

                if ( heroStr != nullptr ) {
                    str.append( "\n(" );
                    str.append( heroStr );
                    str += ')';
                }
            }
        }

        return str;
    }

    std::string showLocalVisitTileInfo( const Maps::Tile & tile )
    {
        std::string str = MP2::StringObject( tile.getMainObjectType( false ) );
        const Heroes * hero = Interface::GetFocusHeroes();
        if ( hero ) {
            str.append( "\n\n" );
            str.append( hero->isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );
        }

        return str;
    }

    std::string showLocalVisitObjectInfo( const MP2::MapObjectType objectType )
    {
        std::string str = MP2::StringObject( objectType );
        const Heroes * hero = Interface::GetFocusHeroes();
        if ( hero ) {
            str.append( "\n\n" );
            str.append( hero->isObjectTypeVisited( objectType ) ? _( "(already visited)" ) : _( "(not visited)" ) );
        }

        return str;
    }

    std::string showObjectVisitInfo( const MP2::MapObjectType objectType, const bool isVisited )
    {
        std::string str = MP2::StringObject( objectType );

        str.append( "\n\n" );
        str.append( isVisited ? _( "(already visited)" ) : _( "(not visited)" ) );

        return str;
    }

    std::string showBarrierInfo( const Maps::Tile & tile, const Kingdom & kingdom )
    {
        std::string str = _( "%{color} Barrier" );
        const int32_t barrierColor = getColorFromTile( tile );
        StringReplace( str, "%{color}", fheroes2::getBarrierColorName( barrierColor ) );

        if ( kingdom.IsVisitTravelersTent( barrierColor ) ) {
            str.append( "\n\n" );
            str.append( _( "(tent visited)" ) );
        }

        return str;
    }

    std::string showTentInfo( const Maps::Tile & tile, const Kingdom & kingdom )
    {
        std::string str = _( "%{color} Tent" );
        const int32_t tentColor = getColorFromTile( tile );
        StringReplace( str, "%{color}", fheroes2::getTentColorName( tentColor ) );

        if ( kingdom.IsVisitTravelersTent( tentColor ) ) {
            str.append( "\n\n" );
            str.append( _( "(already visited)" ) );
        }

        return str;
    }

    std::string showGroundInfo( const Maps::Tile & tile )
    {
        const MP2::MapObjectType objectType = tile.getMainObjectType( false );
        const bool isRoad = tile.isRoad();

        std::string str;
        if ( objectType == MP2::OBJ_COAST ) {
            str = MP2::StringObject( objectType );
        }
        else if ( isRoad ) {
            str = _( "Road" );
        }
        else {
            str = Maps::Ground::String( tile.GetGround() );
        }

        str.append( "\n\n" );

        // Original Editor allows to put an Ultimate Artifact on an invalid tile. So checking tile index solves this issue.
        if ( tile.GoodForUltimateArtifact() || world.GetUltimateArtifact().getPosition() == tile.GetIndex() ) {
            str.append( _( "(digging ok)" ) );
        }
        else {
            str.append( _( "(no digging)" ) );
        }

        const Heroes * hero = Interface::GetFocusHeroes();

        if ( hero ) {
            const uint32_t cost = isRoad ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, hero->GetLevelSkill( Skill::Secondary::PATHFINDING ) );
            if ( cost > 0 ) {
                str += '\n';
                str.append( _( "penalty: %{cost}" ) );
                StringReplace( str, "%{cost}", static_cast<int32_t>( cost ) );
            }
        }

        return str;
    }

    fheroes2::Rect makeRectQuickInfo( const LocalEvent & le, const fheroes2::Sprite & imageBox, const fheroes2::Point & position = fheroes2::Point() )
    {
        if ( position.x > 0 && position.y > 0 ) {
            return { position.x - imageBox.width(), position.y, imageBox.width(), imageBox.height() };
        }

        return Interface::getPopupWindowPosition( le.getMouseCursorPos(), Interface::AdventureMap::Get().getGameArea().GetROI(),
                                                  { imageBox.width(), imageBox.height() } );
    }

    std::string getQuickInfoText( const Maps::Tile & tile )
    {
        const int32_t playerColor = Settings::Get().CurrentColor();
        const MP2::MapObjectType objectType = tile.getMainObjectType( false );

        if ( objectType == MP2::OBJ_ABANDONED_MINE || isCaptureObjectProtected( tile ) ) {
            return showGuardiansInfo( tile, playerColor == getColorFromTile( tile ) );
        }

        const Kingdom & kingdom = world.GetKingdom( playerColor );

        switch ( objectType ) {
        case MP2::OBJ_MONSTER:
            return showMonsterInfo( tile, kingdom.IsTileVisibleFromCrystalBall( tile.GetIndex() ) );

        case MP2::OBJ_EVENT:
        case MP2::OBJ_NONE:
        case MP2::OBJ_COAST:
            return showGroundInfo( tile );

        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DAEMON_CAVE:
        case MP2::OBJ_SPHINX:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_ARTESIAN_SPRING:
        case MP2::OBJ_OBELISK:
            return showObjectVisitInfo( objectType, kingdom.isVisited( tile ) );

        case MP2::OBJ_MAGELLANS_MAPS:
            return showObjectVisitInfo( objectType, kingdom.isVisited( objectType ) );

        case MP2::OBJ_RESOURCE: {
            const Funds funds = getFundsFromTile( tile );
            assert( funds.GetValidItemsCount() == 1 );

            return Resource::String( funds.getFirstValidResource().first );
        }

        case MP2::OBJ_MINE:
            return showMineInfo( tile, playerColor == getColorFromTile( tile ) );

        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_SAWMILL: {
            std::string objectInfo = MP2::StringObject( objectType );
            if ( playerColor == getColorFromTile( tile ) ) {
                const Funds funds = getDailyIncomeObjectResources( tile );
                assert( funds.GetValidItemsCount() == 1 );

                // TODO: we should use the value from funds.
                objectInfo.append( getMineIncomeString( funds.getFirstValidResource().first ) );
            }
            return objectInfo;
        }

        // join army
        case MP2::OBJ_WATCH_TOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREE_HOUSE:
        case MP2::OBJ_ARCHER_HOUSE:
        case MP2::OBJ_GOBLIN_HUT:
        case MP2::OBJ_DWARF_COTTAGE:
        case MP2::OBJ_HALFLING_HOLE:
        case MP2::OBJ_PEASANT_HUT:
        // recruit army
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREE_CITY:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_DESERT_TENT:
        // battle and recruit army
        case MP2::OBJ_DRAGON_CITY:
        case MP2::OBJ_CITY_OF_DEAD:
        case MP2::OBJ_TROLL_BRIDGE:
        case MP2::OBJ_BARROW_MOUNDS:
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_EARTH_ALTAR:
        case MP2::OBJ_WATER_ALTAR:
            return showDwellingInfo( tile, kingdom.isVisited( tile ) );

        case MP2::OBJ_GAZEBO:
        case MP2::OBJ_FORT:
        case MP2::OBJ_XANADU:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_WITCH_DOCTORS_HUT:
        case MP2::OBJ_STANDING_STONES:
            return showLocalVisitTileInfo( tile );

        case MP2::OBJ_MAGIC_WELL:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_OASIS:
        case MP2::OBJ_TEMPLE:
        case MP2::OBJ_BUOY:
        case MP2::OBJ_MERMAID:
        case MP2::OBJ_WATERING_HOLE:
        case MP2::OBJ_ARENA:
        case MP2::OBJ_STABLES:
        case MP2::OBJ_SIRENS:
            return showLocalVisitObjectInfo( objectType );

        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
            return showShrineInfo( tile, kingdom.isVisited( tile ) );

        case MP2::OBJ_WITCHS_HUT:
            return showWitchHutInfo( tile, kingdom.isVisited( tile ) );

        case MP2::OBJ_BARRIER:
            return showBarrierInfo( tile, kingdom );

        case MP2::OBJ_TRAVELLER_TENT:
            return showTentInfo( tile, kingdom );

        case MP2::OBJ_TREE_OF_KNOWLEDGE:
            return showTreeOfKnowledgeInfo( tile, kingdom.isVisited( tile ) );
        // These objects do not have extra text for quick info.
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_CAMPFIRE:
        default:
            return MP2::StringObject( objectType );
        }
    }

    void showQuickInfo( const Castle & castle, const fheroes2::Point & position, const bool showOnRadar, const fheroes2::Rect & areaToRestore )
    {
        const CursorRestorer cursorRestorer( false );

        // Update radar if needed
        RadarUpdater radarUpdater( showOnRadar, castle.GetCenter(), areaToRestore );

        // image box
        const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::QWIKTOWN, 0 );

        LocalEvent & le = LocalEvent::Get();
        fheroes2::Rect cur_rt = makeRectQuickInfo( le, box, position );

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
        const uint32_t flagIcnIndex = fheroes2::getCastleLeftFlagIcnIndex( castle.GetColor() );

        const fheroes2::Point flagOffset( 5, 4 );

        const fheroes2::Sprite & l_flag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex );
        fheroes2::Blit( l_flag, display, dst_pt.x - flagOffset.x - l_flag.width(), dst_pt.y + flagOffset.y );

        const fheroes2::Sprite & r_flag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex + 1 );
        fheroes2::Blit( r_flag, display, dst_pt.x + flagOffset.x + castleIcon.width(), dst_pt.y + flagOffset.y );

        const int currentColor = conf.CurrentColor();
        const Kingdom & kingdom = world.GetKingdom( currentColor );

        const bool isDetailedView = castle.isFriends( currentColor ) || kingdom.IsTileVisibleFromCrystalBall( castle.GetIndex() );
        const uint32_t thievesGuildsCount = kingdom.GetCountThievesGuild();

        text.set( _( "Defenders:" ), fheroes2::FontType::smallWhite() );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
        dst_pt.y += castleIcon.height() + 2;
        text.draw( dst_pt.x, dst_pt.y, display );

        // draw defenders
        if ( isDetailedView || thievesGuildsCount > 0 ) {
            const Army & castleArmy = castle.GetArmy();

            if ( castleArmy.isValid() ) {
                dst_pt.x = cur_rt.x - 1;
                dst_pt.y += 21;

                Army::drawMultipleMonsterLines( castleArmy, dst_pt.x, dst_pt.y, 192, false, isDetailedView, true, thievesGuildsCount );
            }
            else {
                text.set( _( "None" ), fheroes2::FontType::smallWhite() );
                dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
                dst_pt.y += 47;
                text.draw( dst_pt.x, dst_pt.y, display );
            }
        }
        else {
            text.set( _( "Unknown" ), fheroes2::FontType::smallWhite() );
            dst_pt.x = cur_rt.x + ( cur_rt.width - text.width() ) / 2;
            dst_pt.y += 47;
            text.draw( dst_pt.x, dst_pt.y, display );
        }

        display.render();

        // quick info loop
        while ( le.HandleEvents() && le.isMouseRightButtonPressed() )
            ;

        // restore background
        back.restore();

        // Restore radar view
        radarUpdater.restore();

        display.render();
    }

    void showQuickInfo( const HeroBase & hero, const fheroes2::Point & position, const bool showOnRadar, const fheroes2::Rect & areaToRestore,
                        const std::optional<bool> showFullInfo )
    {
        const CursorRestorer cursorRestorer( false );

        // Update radar if needed
        RadarUpdater radarUpdater( showOnRadar, hero.GetCenter(), areaToRestore );

        // image box
        const fheroes2::Sprite & box = fheroes2::AGG::GetICN( ICN::QWIKHERO, 0 );

        LocalEvent & le = LocalEvent::Get();
        fheroes2::Rect cur_rt = makeRectQuickInfo( le, box, position );

        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::ImageRestorer restorer( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
        fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

        cur_rt = fheroes2::Rect( restorer.x() + 28, restorer.y() + 10, 146, 144 );
        fheroes2::Point dst_pt;

        const Settings & conf = Settings::Get();

        const bool isNeutralHero = ( hero.GetColor() == Color::NONE );
        const bool isFullInfo = [&hero, showFullInfo, &conf, isNeutralHero]() {
            if ( showFullInfo ) {
                return *showFullInfo;
            }

            const Kingdom & kingdom = world.GetKingdom( conf.CurrentColor() );
            const bool isFriend = ColorBase( hero.GetColor() ).isFriends( conf.CurrentColor() );
            const bool isUnderIdentifyHeroSpell = kingdom.Modes( Kingdom::IDENTIFYHERO );

            return ( isNeutralHero || isFriend || isUnderIdentifyHeroSpell || kingdom.IsTileVisibleFromCrystalBall( hero.GetIndex() ) );
        }();

        const Heroes * activeHero = dynamic_cast<const Heroes *>( &hero );
        const Captain * activeCaptain = dynamic_cast<const Captain *>( &hero );
        assert( activeHero != nullptr || activeCaptain != nullptr );

        const bool isActiveHero = ( activeHero != nullptr );

        std::string message;
        // hero's name
        if ( isFullInfo && isActiveHero ) {
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

        const fheroes2::Sprite & heroPortraitFrame = fheroes2::AGG::GetICN( conf.isEvilInterfaceEnabled() ? ICN::LOCATORE : ICN::LOCATORS, 22 );

        // mini port heroes
        const fheroes2::Sprite & port = isActiveHero ? activeHero->GetPortrait( PORT_SMALL ) : activeCaptain->GetPortrait( PORT_SMALL );
        if ( !port.empty() ) {
            fheroes2::Blit( heroPortraitFrame, display, cur_rt.x + ( cur_rt.width - heroPortraitFrame.width() ) / 2, cur_rt.y + 12 );
            fheroes2::Blit( port, display, cur_rt.x + ( cur_rt.width - port.width() ) / 2, cur_rt.y + 16 );
        }

        // luck
        if ( isFullInfo ) {
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
        if ( isFullInfo ) {
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
            // Use castle flags to show hero's color flags.
            const uint32_t flagIcnIndex = fheroes2::getCastleLeftFlagIcnIndex( hero.GetColor() );

            const fheroes2::Sprite & l_flag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex );
            dst_pt.x = cur_rt.x + ( cur_rt.width - 40 ) / 2 - l_flag.width();
            fheroes2::Blit( l_flag, display, dst_pt.x, dst_pt.y );

            const fheroes2::Sprite & r_flag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex + 1 );
            dst_pt.x = cur_rt.x + ( cur_rt.width + 40 ) / 2;
            fheroes2::Blit( r_flag, display, dst_pt.x, dst_pt.y );
        }

        const uint16_t statNumberColumn = 89;
        const uint16_t statRow = 11;

        if ( isFullInfo ) {
            // attack
            text.set( _( "Attack:" ), smallWhite );
            dst_pt.x = cur_rt.x + 10;
            dst_pt.y += heroPortraitFrame.height();
            text.fitToOneRow( statNumberColumn );
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( hero.GetAttack() ), smallWhite );
            dst_pt.x += statNumberColumn;
            text.draw( dst_pt.x, dst_pt.y, display );

            // defense
            text.set( _( "Defense:" ), smallWhite );
            dst_pt.x = cur_rt.x + 10;
            dst_pt.y += statRow;
            text.fitToOneRow( statNumberColumn );
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( hero.GetDefense() ), smallWhite );
            dst_pt.x += statNumberColumn;
            text.draw( dst_pt.x, dst_pt.y, display );

            // power
            text.set( _( "Spell Power:" ), smallWhite );
            dst_pt.x = cur_rt.x + 10;
            dst_pt.y += statRow;
            text.fitToOneRow( statNumberColumn );
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( hero.GetPower() ), smallWhite );
            dst_pt.x += statNumberColumn;
            text.draw( dst_pt.x, dst_pt.y, display );

            // knowledge
            text.set( _( "Knowledge:" ), smallWhite );
            dst_pt.x = cur_rt.x + 10;
            dst_pt.y += statRow;
            text.fitToOneRow( statNumberColumn );
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( hero.GetKnowledge() ), smallWhite );
            dst_pt.x += statNumberColumn;
            text.draw( dst_pt.x, dst_pt.y, display );

            // spell point
            text.set( _( "Spell Points:" ), smallWhite );
            dst_pt.x = cur_rt.x + 10;
            dst_pt.y += statRow;
            text.fitToOneRow( statNumberColumn );
            text.draw( dst_pt.x, dst_pt.y, display );

            text.set( std::to_string( hero.GetSpellPoints() ) + "/" + std::to_string( hero.GetMaxSpellPoints() ), smallWhite );
            dst_pt.x += statNumberColumn;
            text.draw( dst_pt.x, dst_pt.y, display );

            // move point
            if ( isActiveHero ) {
                text.set( _( "Move Points:" ), smallWhite );
                dst_pt.x = cur_rt.x + 10;
                dst_pt.y += statRow;
                text.fitToOneRow( statNumberColumn );
                text.draw( dst_pt.x, dst_pt.y, display );

                text.set( std::to_string( activeHero->GetMovePoints() ) + "/" + std::to_string( activeHero->GetMaxMovePoints() ), smallWhite );
                dst_pt.x += statNumberColumn;
                text.draw( dst_pt.x, dst_pt.y, display );
            }

            Army::drawSingleDetailedMonsterLine( hero.GetArmy(), cur_rt.x - 7, cur_rt.y + 118, 160 );
        }
        else {
            // show limited
            Army::drawMultipleMonsterLines( hero.GetArmy(), cur_rt.x - 6, cur_rt.y + 60, 160, false, false );
        }

        display.render();

        // quick info loop
        while ( le.HandleEvents() && le.isMouseRightButtonPressed() )
            ;

        // restore background
        restorer.restore();

        // Restore radar view
        radarUpdater.restore();

        display.render();
    }
}

void Dialog::QuickInfo( const Maps::Tile & tile )
{
    std::string infoString;

    const int32_t playerColor = Settings::Get().CurrentColor();

    if ( ( playerColor != 0 ) && tile.isFog( playerColor ) ) {
        infoString = _( "Uncharted Territory" );
    }
    else {
        const int32_t mainTileIndex = Maps::Tile::getIndexOfMainTile( tile );

        if ( mainTileIndex != -1 ) {
            infoString = getQuickInfoText( world.getTile( mainTileIndex ) );
        }
        else {
            infoString = getQuickInfoText( tile );
        }
    }

    outputInTextSupportMode( tile, infoString );

    Interface::displayStandardPopupWindow( std::move( infoString ), Interface::AdventureMap::Get().getGameArea().GetROI() );
}

void Dialog::QuickInfo( const Castle & castle )
{
    showQuickInfo( castle, {}, false, {} );
}

void Dialog::QuickInfo( const HeroBase & hero, const std::optional<bool> showFullInfo /* = {} */ )
{
    showQuickInfo( hero, {}, false, {}, showFullInfo );
}

void Dialog::QuickInfoWithIndicationOnRadar( const Castle & castle, const fheroes2::Rect & areaToRestore )
{
    showQuickInfo( castle, {}, true, areaToRestore );
}

void Dialog::QuickInfoWithIndicationOnRadar( const HeroBase & hero, const fheroes2::Rect & areaToRestore )
{
    showQuickInfo( hero, {}, true, areaToRestore, {} );
}

void Dialog::QuickInfoAtPosition( const Castle & castle, const fheroes2::Point & position )
{
    showQuickInfo( castle, position, true, {} );
}

void Dialog::QuickInfoAtPosition( const HeroBase & hero, const fheroes2::Point & position )
{
    showQuickInfo( hero, position, true, {}, {} );
}
