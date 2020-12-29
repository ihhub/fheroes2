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

#include "agg.h"
#include "army.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "maps.h"
#include "monster.h"
#include "profit.h"
#include "race.h"
#include "settings.h"
#include "spell.h"
#include "text.h"
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
        res.append( GetString( value ) );
        res.append( ")" );
    }

    return res;
}

std::string ShowGuardiansInfo( const Maps::Tiles & tile, int scoute )
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

    if ( troop.isValid() ) {
        str.append( "\n" );
        str.append( _( "guarded by %{count} of %{monster}" ) );

        StringReplace( str, "%{monster}", StringLower( troop.GetMultiName() ) );
        StringReplace( str, "%{count}", Game::CountScoute( troop.GetCount(), scoute ) );
    }

    return str;
}

std::string ShowMonsterInfo( const Maps::Tiles & tile, int scoute )
{
    const Troop & troop = tile.QuantityTroop();

    if ( scoute ) {
        std::string str = "%{count} %{monster}";
        StringReplace( str, "%{count}", Game::CountScoute( troop.GetCount(), scoute ) );
        StringReplace( str, "%{monster}", StringLower( troop.GetMultiName() ) );
        return str;
    }
    else {
        return Army::TroopSizeString( troop );
    }
}

std::string ShowArtifactInfo( const Maps::Tiles & tile, bool show )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    if ( show ) {
        str.append( "\n(" );
        str.append( tile.QuantityArtifact().GetName() );
        str.append( ")" );
    }

    return str;
}

std::string ShowResourceInfo( const Maps::Tiles & tile, int scoute )
{
    std::string str;

    if ( MP2::OBJ_RESOURCE == tile.GetObject() ) {
        str = Resource::String( tile.GetQuantity1() );
        if ( scoute ) {
            const ResourceCount & rc = tile.QuantityResourceCount();
            str.append( ": " );
            str.append( Game::CountScoute( rc.second, scoute ) );
        }
    }
    else {
        str = MP2::StringObject( tile.GetObject() );
        if ( scoute ) {
            const Funds & funds = tile.QuantityFunds();

            str.append( "\n(" );
            str.append( Resource::String( Resource::GOLD ) );

            str.append( ": " );
            str.append( Game::CountScoute( funds.gold, scoute ) );
            str.append( "\n" );

            const ResourceCount & rc = tile.QuantityResourceCount();
            str.append( Resource::String( rc.first ) );

            str.append( ": " );
            str.append( Game::CountScoute( rc.second, scoute ) );
            str.append( ")" );
        }
    }

    return str;
}

std::string ShowDwellingInfo( const Maps::Tiles & tile, int scoute )
{
    std::string str = MP2::StringObject( tile.GetObject() );

    if ( scoute ) {
        str.append( "\n" );
        const Troop & troop = tile.QuantityTroop();
        if ( troop.isValid() ) {
            str.append( _( "(available: %{count})" ) );
            StringReplace( str, "%{count}", Game::CountScoute( troop.GetCount(), scoute ) );
        }
        else
            str.append( "(empty)" );
    }

    return str;
}

std::string ShowShrineInfo( const Maps::Tiles & tile, const Heroes * hero, int scoute )
{
    std::string str = MP2::StringObject( tile.GetObject() );
    bool show = false;

    switch ( tile.GetObject() ) {
    case MP2::OBJ_SHRINE1:
        show = scoute >= Skill::Level::BASIC;
        break;
    case MP2::OBJ_SHRINE2:
        show = scoute >= Skill::Level::ADVANCED;
        break;
    case MP2::OBJ_SHRINE3:
        show = scoute == Skill::Level::EXPERT;
        break;
    default:
        break;
    }

    if ( show ) {
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

std::string ShowWitchHutInfo( const Maps::Tiles & tile, const Heroes * hero, bool show )
{
    std::string str = MP2::StringObject( tile.GetObject() );

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
        str.append( "\n" );
        str.append( hero->isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );
    }

    return str;
}

std::string ShowLocalVisitObjectInfo( const Maps::Tiles & tile, const Heroes * hero )
{
    std::string str = MP2::StringObject( tile.GetObject() );
    if ( hero ) {
        str.append( "\n" );
        str.append( hero->isObjectTypeVisited( tile.GetObject() ) ? _( "(already visited)" ) : _( "(not visited)" ) );
    }

    return str;
}

std::string ShowGlobalVisitInfo( const Maps::Tiles & tile, const Kingdom & kingdom )
{
    std::string str = MP2::StringObject( tile.GetObject() );
    str.append( "\n" );
    str.append( kingdom.isVisited( tile ) ? _( "(already visited)" ) : _( "(not visited)" ) );

    return str;
}

std::string ShowGlobalVisitInfo( const Maps::Tiles & tile, const Kingdom & kingdom, bool ext )
{
    std::string str = MP2::StringObject( tile.GetObject() );
    if ( ext && kingdom.isVisited( tile ) ) {
        str.append( "\n" );
        str.append( _( "(already visited)" ) );
    }

    return str;
}

std::string ShowBarrierTentInfo( const Maps::Tiles & tile, const Kingdom & kingdom )
{
    std::string str = BarrierColor::String( tile.QuantityColor() );
    str.append( " " );
    str.append( MP2::StringObject( tile.GetObject() ) );

    if ( MP2::OBJ_TRAVELLERTENT == tile.GetObject() && kingdom.IsVisitTravelersTent( tile.QuantityColor() ) ) {
        str.append( "\n" );
        str.append( _( "(already visited)" ) );
    }

    return str;
}

std::string ShowGroundInfo( const Maps::Tiles & tile, bool show, const Heroes * hero )
{
    std::string str = tile.isRoad() ? _( "Road" ) : Maps::Ground::String( tile.GetGround() );

    if ( show && hero ) {
        int dir = Maps::GetDirection( hero->GetIndex(), tile.GetIndex() );
        if ( dir != Direction::UNKNOWN ) {
            uint32_t cost = tile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, hero->GetLevelSkill( Skill::Secondary::PATHFINDING ) );

            if ( cost ) {
                str.append( "\n" );
                str.append( _( "penalty: %{cost}" ) );
                StringReplace( str, "%{cost}", cost );
            }
        }
    }

    return str;
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
        const Maps::Tiles * center = NULL;

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

    const Settings & settings = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    // preload
    const int qwikinfo = ICN::QWIKINFO;

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( qwikinfo, 0 );
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const Rect ar( gamearea.GetROI() );

    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    fheroes2::Rect pos;
    const s32 mx = mp.x;
    const s32 my = mp.y;

    if ( mx <= ar.x + ar.w / 2 && my <= ar.y + ar.h / 2 ) { // top left
        pos = fheroes2::Rect( mx, my + TILEWIDTH / 2, box.width(), box.height() );
    }
    else if ( mx > ar.x + ar.w / 2 && my <= ar.y + ar.h / 2 ) { // top right
        pos = fheroes2::Rect( mx - box.width() - TILEWIDTH / 2, my + TILEWIDTH / 2, box.width(), box.height() );
    }
    else if ( mx <= ar.x + ar.w / 2 && my > ar.y + ar.h / 2 ) { // bottom left
        pos = fheroes2::Rect( mx, my - box.height(), box.width(), box.height() );
    }
    else { // bottom right
        pos = fheroes2::Rect( mx - box.width() - TILEWIDTH / 2, my - box.height(), box.width(), box.height() );
    }

    fheroes2::ImageRestorer restorer( display, pos.x, pos.y, pos.width, pos.height );
    fheroes2::Blit( box, display, pos.x, pos.y );

    std::string name_object;

    const Heroes * from_hero = Interface::GetFocusHeroes();
    const Kingdom & kingdom = world.GetKingdom( settings.CurrentColor() );
    const int scoute = from_hero ? from_hero->CanScouteTile( tile.GetIndex() ) : 0;
    const bool show = settings.ExtWorldShowVisitedContent();

    if ( tile.isFog( settings.CurrentColor() ) )
        name_object = _( "Unchartered Territory" );
    else
        // check guardians mine
        if ( MP2::OBJ_ABANDONEDMINE == objectType || tile.CaptureObjectIsProtection() ) {
        name_object = ShowGuardiansInfo( tile, ( settings.CurrentColor() == tile.QuantityColor() ? Skill::Level::EXPERT : scoute ) );
    }
    else
        switch ( objectType ) {
        case MP2::OBJ_MONSTER:
            name_object = ShowMonsterInfo( tile, scoute );
            break;

        case MP2::OBJ_EVENT:
        case MP2::OBJ_ZERO:
            name_object = ShowGroundInfo( tile, show, from_hero );
            break;

        case MP2::OBJ_DERELICTSHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DAEMONCAVE:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEANTO:
            name_object = ShowGlobalVisitInfo( tile, kingdom, show );
            break;

        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_MAGICGARDEN:
            name_object = Settings::Get().ExtWorldExtObjectsCaptured() ? MP2::StringObject( objectType ) : ShowGlobalVisitInfo( tile, kingdom, show );
            break;

        case MP2::OBJ_CAMPFIRE:
            name_object = ShowResourceInfo( tile, scoute );
            break;

        case MP2::OBJ_RESOURCE:
            name_object = ShowResourceInfo( tile, scoute );
            break;

        case MP2::OBJ_ARTIFACT:
            name_object = ShowArtifactInfo( tile, scoute != 0 );
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
            name_object = ShowDwellingInfo( tile, ( kingdom.isVisited( tile ) ? Skill::Level::EXPERT : scoute ) );
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
            name_object = ShowShrineInfo( tile, from_hero, ( show && kingdom.isVisited( tile ) ? Skill::Level::EXPERT : scoute ) );
            break;

        case MP2::OBJ_WITCHSHUT:
            name_object = ShowWitchHutInfo( tile, from_hero, ( ( show && kingdom.isVisited( tile ) ) || scoute == Skill::Level::EXPERT ) );
            break;

        case MP2::OBJ_OBELISK:
            name_object = ShowGlobalVisitInfo( tile, kingdom );
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

    cursor.Show();
    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    restorer.restore();
    display.render();
}

void Dialog::QuickInfo( const Castle & castle )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const int qwiktown = ICN::QWIKTOWN;

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( qwiktown, 0 );
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const Rect ar( gamearea.GetROI() );

    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    fheroes2::Rect cur_rt;
    const s32 mx = ( ( mp.x - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;
    const s32 my = ( ( mp.y - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;

    if ( mx <= ar.x + ar.w / 2 && my <= ar.y + ar.h / 2 ) { // top left
        cur_rt = fheroes2::Rect( mx + TILEWIDTH, my + TILEWIDTH, box.width(), box.height() );
    }
    else if ( mx > ar.x + ar.w / 2 && my <= ar.y + ar.h / 2 ) { // top right
        cur_rt = fheroes2::Rect( mx - box.width(), my + TILEWIDTH, box.width(), box.height() );
    }
    else if ( mx <= ar.x + ar.w / 2 && my > ar.y + ar.h / 2 ) { // bottom left
        cur_rt = fheroes2::Rect( mx + TILEWIDTH, my - box.height(), box.width(), box.height() );
    }
    else { // bottom right
        cur_rt = fheroes2::Rect( mx - box.width(), my - box.height(), box.width(), box.height() );
    }

    fheroes2::ImageRestorer back( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( cur_rt.x + 28, cur_rt.y + 9, 178, 140 );
    fheroes2::Point dst_pt;
    Text text;

    // castle name
    text.Set( castle.GetName(), Font::SMALL );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
    dst_pt.y = cur_rt.y;
    text.Blit( dst_pt.x, dst_pt.y );

    u32 index = 0;

    switch ( castle.GetRace() ) {
    case Race::KNGT:
        index = ( castle.isCastle() ? 9 : 15 );
        break;
    case Race::BARB:
        index = ( castle.isCastle() ? 10 : 16 );
        break;
    case Race::SORC:
        index = ( castle.isCastle() ? 11 : 17 );
        break;
    case Race::WRLK:
        index = ( castle.isCastle() ? 12 : 18 );
        break;
    case Race::WZRD:
        index = ( castle.isCastle() ? 13 : 19 );
        break;
    case Race::NECR:
        index = ( castle.isCastle() ? 14 : 20 );
        break;
    default:
        DEBUG( DBG_GAME, DBG_WARN, "unknown race" );
        return;
    }

    // castle icon
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::LOCATORS, index );

    dst_pt.x = cur_rt.x + ( cur_rt.width - sprite.width() ) / 2;
    dst_pt.y += 15;
    fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );

    // color flags
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

    const fheroes2::Sprite & l_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index );
    dst_pt.x = cur_rt.x + ( cur_rt.width - 60 ) / 2 - l_flag.width();
    fheroes2::Blit( l_flag, display, dst_pt.x, dst_pt.y );

    const fheroes2::Sprite & r_flag = fheroes2::AGG::GetICN( ICN::FLAG32, index + 1 );
    dst_pt.x = cur_rt.x + ( cur_rt.width + 60 ) / 2;
    fheroes2::Blit( r_flag, display, dst_pt.x, dst_pt.y );

    // info
    text.Set( _( "Defenders:" ) );
    dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
    dst_pt.y += sprite.height() + 5;
    text.Blit( dst_pt.x, dst_pt.y );

    //
    u32 count = castle.GetArmy().GetCount();
    const Settings & conf = Settings::Get();

    const Heroes * from_hero = Interface::GetFocusHeroes();
    const Heroes * guardian = castle.GetHeroes().Guard();

    const int currentColor = conf.CurrentColor();
    const int thievesGuildCount = world.GetKingdom( currentColor ).GetCountThievesGuild();

    // draw guardian portrait
    if ( guardian &&
         // my  colors
         ( castle.GetColor() == currentColor ||
           // show guardians (scouting: advanced)
           ( from_hero && Skill::Level::ADVANCED <= from_hero->GetSecondaryValues( Skill::Secondary::SCOUTING ) ) ) ) {
        // heroes name
        text.Set( guardian->GetName(), Font::SMALL );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += 10;
        text.Blit( dst_pt.x, dst_pt.y );

        // mini port heroes
        fheroes2::Image port = guardian->GetPortrait( PORT_SMALL );
        if ( !port.empty() ) {
            dst_pt.x = cur_rt.x + ( cur_rt.width - port.width() ) / 2;
            dst_pt.y += 15;
            fheroes2::Blit( port, display, dst_pt.x, dst_pt.y );
        }
    }

    // draw defenders
    if ( !count ) {
        text.Set( _( "None" ) );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += 45;
        text.Blit( dst_pt.x, dst_pt.y );
    }
    else if ( castle.isFriends( currentColor ) ) {
        // show all
        Army::DrawMonsterLines( castle.GetArmy(), cur_rt.x - 5, cur_rt.y + 62, 192, Skill::Level::EXPERT, true, true );
    }
    // draw enemy castle defenders, dependent on thieves guild count
    else if ( thievesGuildCount == 0 ) {
        text.Set( _( "Unknown" ) );
        dst_pt.x = cur_rt.x + ( cur_rt.width - text.w() ) / 2;
        dst_pt.y += 45;
        text.Blit( dst_pt.x, dst_pt.y );
    }
    else {
        // show limited
        Army::DrawMonsterLines( castle.GetArmy(), cur_rt.x - 5, cur_rt.y + 62, 192, thievesGuildCount, false, false );
    }

    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    back.restore();
    display.render();
}

void Dialog::QuickInfo( const Heroes & hero )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const int qwikhero = ICN::QWIKHERO;

    // image box
    const fheroes2::Sprite & box = fheroes2::AGG::GetICN( qwikhero, 0 );
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const Rect ar( gamearea.GetROI() );

    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    fheroes2::Rect cur_rt;
    const s32 mx = ( ( mp.x - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;
    const s32 my = ( ( mp.y - BORDERWIDTH ) / TILEWIDTH ) * TILEWIDTH;

    if ( mx <= ar.x + ar.w / 2 && my <= ar.y + ar.h / 2 ) { // top left
        cur_rt = fheroes2::Rect( mx + TILEWIDTH, my + TILEWIDTH, box.width(), box.height() );
    }
    else if ( mx > ar.x + ar.w / 2 && my <= ar.y + ar.h / 2 ) { // top right
        cur_rt = fheroes2::Rect( mx - box.width(), my + TILEWIDTH, box.width(), box.height() );
    }
    else if ( mx <= ar.x + ar.w / 2 && my > ar.y + ar.h / 2 ) { // bottom left
        cur_rt = fheroes2::Rect( mx + TILEWIDTH, my - box.height(), box.width(), box.height() );
    }
    else { // bottom right
        cur_rt = fheroes2::Rect( mx - box.width(), my - box.height(), box.width(), box.height() );
    }

    fheroes2::ImageRestorer restorer( display, cur_rt.x, cur_rt.y, cur_rt.width, cur_rt.height );
    fheroes2::Blit( box, display, cur_rt.x, cur_rt.y );

    cur_rt = fheroes2::Rect( restorer.x() + 28, restorer.y() + 10, 146, 144 );
    fheroes2::Point dst_pt;
    Text text;
    std::string message;

    const bool isFriend = hero.isFriends( conf.CurrentColor() );
    const bool isUnderIdentifyHeroSpell = world.GetKingdom( conf.CurrentColor() ).Modes( Kingdom::IDENTIFYHERO );
    const bool showFullInfo = isFriend || isUnderIdentifyHeroSpell;

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
    fheroes2::Image port = hero.GetPortrait( PORT_SMALL );
    if ( !port.empty() ) {
        dst_pt.x = cur_rt.x + ( cur_rt.width - port.width() ) / 2;
        dst_pt.y = cur_rt.y + 13;
        fheroes2::Blit( port, display, dst_pt.x, dst_pt.y );
    }

    // luck
    if ( showFullInfo ) {
        const s32 luck = hero.GetLuckWithModificators( NULL );
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
        const s32 morale = hero.GetMoraleWithModificators( NULL );
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

        text.Set( GetString( hero.GetAttack() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // defense
        text.Set( std::string( _( "Defense" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( GetString( hero.GetDefense() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // power
        text.Set( std::string( _( "Spell Power" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( GetString( hero.GetPower() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // knowledge
        text.Set( std::string( _( "Knowledge" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( GetString( hero.GetKnowledge() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // spell point
        text.Set( std::string( _( "Spell Points" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( GetString( hero.GetSpellPoints() ) + "/" + GetString( hero.GetMaxSpellPoints() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        // move point
        text.Set( std::string( _( "Move Points" ) ) + ":" );
        dst_pt.x = cur_rt.x + 10;
        dst_pt.y += 12;
        text.Blit( dst_pt.x, dst_pt.y );

        text.Set( GetString( hero.GetMobilityIndexSprite() ) + "/" + GetString( hero.GetMovePoints() ) + "/" + GetString( hero.GetMaxMovePoints() ) );
        dst_pt.x += 75;
        text.Blit( dst_pt.x, dst_pt.y );

        Army::DrawMons32Line( hero.GetArmy(), cur_rt.x - 7, cur_rt.y + 116, 160 );
    }
    else {
        // show limited
        Army::DrawMonsterLines( hero.GetArmy(), cur_rt.x - 6, cur_rt.y + 60, 160, Skill::Level::NONE, false, true );
    }

    cursor.Show();
    display.render();

    // quick info loop
    while ( le.HandleEvents() && le.MousePressRight() )
        ;

    // restore background
    restorer.restore();
    display.render();
}
